/**
 * @file lv_draw_dma2d.c
 * @brief STM32 DMA2D Draw Unit Implementation for LVGL 9.4
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_draw_dma2d.h"

#if LV_USE_DRAW_DMA2D

#include LV_DRAW_DMA2D_HAL_INCLUDE
#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_draw_unit_t base_unit;
    bool dma2d_busy;
#if LV_USE_OS
    lv_thread_sync_t sync;
#endif
} dma2d_draw_unit_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int32_t dma2d_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static int32_t dma2d_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static void dma2d_execute_fill(dma2d_draw_unit_t * u, const lv_draw_fill_dsc_t * dsc,
                               const lv_area_t * coords);
static void dma2d_execute_image(dma2d_draw_unit_t * u, const lv_draw_image_dsc_t * dsc,
                                const lv_area_t * coords);
static void dma2d_wait_for_completion(dma2d_draw_unit_t * u);
static uint32_t lv_cf_to_dma2d_cf(lv_color_format_t cf);

/**********************
 *  STATIC VARIABLES
 **********************/
static dma2d_draw_unit_t dma2d_unit;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_draw_dma2d_init(void)
{
    memset(&dma2d_unit, 0, sizeof(dma2d_unit));
    
    /* Initialize base draw unit */
    lv_draw_unit_init(&dma2d_unit.base_unit);
    
    dma2d_unit.base_unit.evaluate_cb = dma2d_evaluate;
    dma2d_unit.base_unit.dispatch_cb = dma2d_dispatch;
    
#if LV_USE_OS
    lv_thread_sync_init(&dma2d_unit.sync);
#endif

#if LV_USE_DRAW_DMA2D_INTERRUPT
    /* Enable DMA2D transfer complete interrupt */
    HAL_NVIC_SetPriority(DMA2D_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(DMA2D_IRQn);
    
    /* Enable DMA2D transfer complete interrupt in peripheral */
    DMA2D->CR |= DMA2D_CR_TCIE;
#endif

    LV_LOG_INFO("DMA2D draw unit initialized");
}

void lv_draw_dma2d_deinit(void)
{
#if LV_USE_DRAW_DMA2D_INTERRUPT
    HAL_NVIC_DisableIRQ(DMA2D_IRQn);
    DMA2D->CR &= ~DMA2D_CR_TCIE;
#endif

#if LV_USE_OS
    lv_thread_sync_delete(&dma2d_unit.sync);
#endif

    LV_LOG_INFO("DMA2D draw unit deinitialized");
}

#if LV_USE_DRAW_DMA2D_INTERRUPT
void lv_draw_dma2d_transfer_complete_interrupt_handler(void)
{
    /* Check if transfer complete flag is set */
    if (DMA2D->ISR & DMA2D_ISR_TCIF) {
        /* Clear the flag */
        DMA2D->IFCR |= DMA2D_IFCR_CTCIF;
        
        dma2d_unit.dma2d_busy = false;
        
#if LV_USE_OS
        lv_thread_sync_signal(&dma2d_unit.sync);
#endif
    }
}
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t dma2d_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);
    
    /* Check if this is a task we can handle */
    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            /* We can handle simple fills without gradient or radius */
            {
                const lv_draw_fill_dsc_t * dsc = task->draw_dsc;
                if (dsc->grad.dir == LV_GRAD_DIR_NONE && dsc->radius == 0) {
                    return 80;  /* Higher score = higher priority */
                }
            }
            break;
            
        case LV_DRAW_TASK_TYPE_IMAGE:
            /* We can handle simple image blits without transforms */
            {
                const lv_draw_image_dsc_t * dsc = task->draw_dsc;
                if (dsc->rotation == 0 && dsc->scale_x == LV_SCALE_NONE && 
                    dsc->scale_y == LV_SCALE_NONE) {
                    return 80;
                }
            }
            break;
            
        default:
            break;
    }
    
    return 0;  /* Cannot handle this task */
}

static int32_t dma2d_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    dma2d_draw_unit_t * u = (dma2d_draw_unit_t *)draw_unit;
    
    /* Process tasks from the layer */
    lv_draw_task_t * task = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_DMA2D);
    
    if (task == NULL) {
        return 0;
    }
    
    /* Wait for previous DMA2D operation to complete */
    dma2d_wait_for_completion(u);
    
    /* Execute the task based on type */
    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            dma2d_execute_fill(u, task->draw_dsc, &task->area);
            break;
            
        case LV_DRAW_TASK_TYPE_IMAGE:
            dma2d_execute_image(u, task->draw_dsc, &task->area);
            break;
            
        default:
            LV_LOG_WARN("Unsupported task type: %d", task->type);
            break;
    }
    
    /* Mark task as completed */
    task->state = LV_DRAW_TASK_STATE_READY;
    
    return 1;  /* Return non-zero if we dispatched a task */
}

static void dma2d_execute_fill(dma2d_draw_unit_t * u, const lv_draw_fill_dsc_t * dsc,
                               const lv_area_t * coords)
{
    /* Get layer information */
    lv_layer_t * layer = dsc->base.layer;
    lv_color_format_t cf = layer->color_format;
    uint32_t dma2d_cf = lv_cf_to_dma2d_cf(cf);
    
    if (dma2d_cf == 0) {
        LV_LOG_ERROR("Unsupported color format for DMA2D: %d", cf);
        return;
    }
    
    /* Calculate destination parameters */
    uint32_t dest_addr = (uint32_t)layer->buf + 
                         (coords->y1 * layer->buf_stride + coords->x1) * lv_color_format_get_bpp(cf) / 8;
    uint32_t width = lv_area_get_width(coords);
    uint32_t height = lv_area_get_height(coords);
    uint32_t offset = layer->buf_stride - width;
    
    /* Convert LVGL color to DMA2D color */
    lv_color32_t c32 = lv_color_to_32(dsc->color, dsc->opa);
    uint32_t color = 0;
    
    if (dma2d_cf == DMA2D_OUTPUT_RGB565) {
        color = ((c32.red & 0xF8) << 8) | ((c32.green & 0xFC) << 3) | (c32.blue >> 3);
    } else if (dma2d_cf == DMA2D_OUTPUT_RGB888) {
        color = (c32.red << 16) | (c32.green << 8) | c32.blue;
    } else if (dma2d_cf == DMA2D_OUTPUT_ARGB8888) {
        color = (c32.alpha << 24) | (c32.red << 16) | (c32.green << 8) | c32.blue;
    }
    
    /* Configure DMA2D for register-to-memory transfer (fill) */
    DMA2D->CR = 0x00030000;  /* Mode: Register-to-memory */
    DMA2D->OPFCCR = dma2d_cf;
    DMA2D->OCOLR = color;
    DMA2D->OMAR = dest_addr;
    DMA2D->OOR = offset;
    DMA2D->NLR = (width << 16) | height;
    
    /* Start transfer */
    u->dma2d_busy = true;
    DMA2D->CR |= DMA2D_CR_START;
    
    /* Wait for completion if not using interrupts */
#if !LV_USE_DRAW_DMA2D_INTERRUPT
    while (DMA2D->CR & DMA2D_CR_START);
    u->dma2d_busy = false;
#endif
}

static void dma2d_execute_image(dma2d_draw_unit_t * u, const lv_draw_image_dsc_t * dsc,
                                const lv_area_t * coords)
{
    /* Simplified image blit - full implementation would handle all cases */
    LV_LOG_WARN("DMA2D image drawing not fully implemented yet");
    LV_UNUSED(u);
    LV_UNUSED(dsc);
    LV_UNUSED(coords);
}

static void dma2d_wait_for_completion(dma2d_draw_unit_t * u)
{
    if (!u->dma2d_busy) {
        return;
    }
    
#if LV_USE_DRAW_DMA2D_INTERRUPT && LV_USE_OS
    /* Wait for interrupt signal */
    lv_thread_sync_wait(&u->sync);
#else
    /* Polling wait */
    while (DMA2D->CR & DMA2D_CR_START);
    u->dma2d_busy = false;
#endif
}

static uint32_t lv_cf_to_dma2d_cf(lv_color_format_t cf)
{
    switch (cf) {
        case LV_COLOR_FORMAT_RGB565:
            return DMA2D_OUTPUT_RGB565;
        case LV_COLOR_FORMAT_RGB888:
            return DMA2D_OUTPUT_RGB888;
        case LV_COLOR_FORMAT_ARGB8888:
            return DMA2D_OUTPUT_ARGB8888;
        default:
            return 0;
    }
}

#endif /* LV_USE_DRAW_DMA2D */

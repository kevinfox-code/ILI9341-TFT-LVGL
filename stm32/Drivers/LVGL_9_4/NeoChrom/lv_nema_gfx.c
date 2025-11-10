/**
 * @file lv_nema_gfx.c
 * @brief STM32 NeoChrom GPU Driver Implementation for LVGL 9.4
 */

/*********************
 *      INCLUDES
 *********************/
#include "lv_nema_gfx.h"

#if LV_USE_NEMA_GFX

#include <string.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
    lv_draw_unit_t base_unit;
    nema_cmdlist_t cl;
    bool gpu_busy;
#if LV_USE_OS
    lv_thread_sync_t sync;
#endif
} nema_draw_unit_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static int32_t nema_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task);
static int32_t nema_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer);
static void nema_execute_fill(nema_draw_unit_t * u, const lv_draw_fill_dsc_t * dsc,
                              const lv_area_t * coords);
static void nema_execute_image(nema_draw_unit_t * u, const lv_draw_image_dsc_t * dsc,
                               const lv_area_t * coords);
static void nema_wait_for_completion(nema_draw_unit_t * u);

/* HAL implementation functions */
#if LV_USE_NEMA_HAL == LV_NEMA_HAL_STM32_DEFAULT
static void * nema_hal_malloc(size_t size);
static void nema_hal_free(void * ptr);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static nema_draw_unit_t nema_unit;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_nema_gfx_init(void)
{
    memset(&nema_unit, 0, sizeof(nema_unit));
    
    /* Initialize NeoChrom core */
    if (nema_init() != 0) {
        LV_LOG_ERROR("Failed to initialize NeoChrom");
        return;
    }
    
    /* Create command list */
    nema_unit.cl = nema_cl_create();
    if (nema_unit.cl.bo.base_virt == NULL) {
        LV_LOG_ERROR("Failed to create NeoChrom command list");
        nema_deinit();
        return;
    }
    
    /* Bind command list */
    nema_cl_bind(&nema_unit.cl);
    
    /* Initialize base draw unit */
    lv_draw_unit_init(&nema_unit.base_unit);
    nema_unit.base_unit.evaluate_cb = nema_evaluate;
    nema_unit.base_unit.dispatch_cb = nema_dispatch;
    
#if LV_USE_OS
    lv_thread_sync_init(&nema_unit.sync);
#endif

    LV_LOG_INFO("NeoChrom draw unit initialized");
}

void lv_nema_gfx_deinit(void)
{
    nema_wait_for_completion(&nema_unit);
    
    /* Destroy command list */
    nema_cl_destroy(&nema_unit.cl);
    
    /* Deinitialize NeoChrom */
    nema_deinit();
    
#if LV_USE_OS
    lv_thread_sync_delete(&nema_unit.sync);
#endif

    LV_LOG_INFO("NeoChrom draw unit deinitialized");
}

#if LV_USE_NEMA_VG
void lv_nema_vg_init(void)
{
    /* Initialize NeoChrom VG */
    if (nema_vg_init(LV_NEMA_GFX_MAX_RESX, LV_NEMA_GFX_MAX_RESY) != 0) {
        LV_LOG_ERROR("Failed to initialize NeoChrom VG");
        return;
    }
    
    LV_LOG_INFO("NeoChrom VG initialized for %dx%d", 
                LV_NEMA_GFX_MAX_RESX, LV_NEMA_GFX_MAX_RESY);
}

void lv_nema_vg_deinit(void)
{
    nema_vg_deinit();
    LV_LOG_INFO("NeoChrom VG deinitialized");
}
#endif

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int32_t nema_evaluate(lv_draw_unit_t * draw_unit, lv_draw_task_t * task)
{
    LV_UNUSED(draw_unit);
    
    /* Evaluate if we can handle this task */
    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            {
                const lv_draw_fill_dsc_t * dsc = task->draw_dsc;
                /* Handle simple fills and gradients */
                if (dsc->radius == 0) {
                    return 90;  /* High priority for NeoChrom */
                }
            }
            break;
            
        case LV_DRAW_TASK_TYPE_IMAGE:
            {
                const lv_draw_image_dsc_t * dsc = task->draw_dsc;
                /* Check if it's a TSC image or standard format */
                lv_image_header_t * header = (lv_image_header_t *)dsc->src;
                if (header != NULL) {
                    /* TSC formats are handled natively */
                    if (header->cf >= LV_COLOR_FORMAT_NEMA_TSC4 &&
                        header->cf <= LV_COLOR_FORMAT_NEMA_TSC12A) {
                        return 100;  /* Highest priority for TSC */
                    }
                    /* Standard formats with transforms */
                    if (dsc->rotation != 0 || dsc->scale_x != LV_SCALE_NONE) {
                        return 85;
                    }
                    return 70;  /* Lower priority for simple blits */
                }
            }
            break;
            
#if LV_USE_NEMA_VG
        case LV_DRAW_TASK_TYPE_VECTOR:
            /* NeoChrom VG handles vector graphics */
            return 100;
#endif
            
        default:
            break;
    }
    
    return 0;
}

static int32_t nema_dispatch(lv_draw_unit_t * draw_unit, lv_layer_t * layer)
{
    nema_draw_unit_t * u = (nema_draw_unit_t *)draw_unit;
    
    /* Get next available task */
    lv_draw_task_t * task = lv_draw_get_next_available_task(layer, NULL, DRAW_UNIT_ID_NEMA);
    
    if (task == NULL) {
        return 0;
    }
    
    /* Wait for previous operation */
    nema_wait_for_completion(u);
    
    /* Bind command list */
    nema_cl_bind(&u->cl);
    
    /* Execute based on task type */
    switch (task->type) {
        case LV_DRAW_TASK_TYPE_FILL:
            nema_execute_fill(u, task->draw_dsc, &task->area);
            break;
            
        case LV_DRAW_TASK_TYPE_IMAGE:
            nema_execute_image(u, task->draw_dsc, &task->area);
            break;
            
        default:
            LV_LOG_WARN("Unsupported task type: %d", task->type);
            break;
    }
    
    /* Submit command list */
    nema_cl_submit(&u->cl);
    u->gpu_busy = true;
    
    /* Mark task as ready */
    task->state = LV_DRAW_TASK_STATE_READY;
    
    return 1;
}

static void nema_execute_fill(nema_draw_unit_t * u, const lv_draw_fill_dsc_t * dsc,
                              const lv_area_t * coords)
{
    LV_UNUSED(u);
    
    /* Get layer information */
    lv_layer_t * layer = dsc->base.layer;
    
    /* Set destination */
    nema_bind_dst_tex((uintptr_t)layer->buf, layer->buf_stride, 
                      lv_area_get_height(&layer->buf_area),
                      NEMA_RGB565, -1);  /* Adjust format as needed */
    
    /* Convert color */
    lv_color32_t c32 = lv_color_to_32(dsc->color, dsc->opa);
    uint32_t color = nema_rgba(c32.red, c32.green, c32.blue, c32.alpha);
    
    /* Fill rectangle */
    if (dsc->grad.dir == LV_GRAD_DIR_NONE) {
        /* Solid fill */
        nema_set_blend_fill(NEMA_BL_SIMPLE);
        nema_fill_rect(coords->x1, coords->y1, 
                      lv_area_get_width(coords), lv_area_get_height(coords), 
                      color);
    } else {
        /* Gradient fill - NeoChrom supports this */
        /* Implementation would use nema_fill_rect_grad */
        LV_LOG_WARN("Gradient fill not yet implemented");
    }
}

static void nema_execute_image(nema_draw_unit_t * u, const lv_draw_image_dsc_t * dsc,
                               const lv_area_t * coords)
{
    LV_UNUSED(u);
    LV_UNUSED(dsc);
    LV_UNUSED(coords);
    
    /* Image blitting implementation */
    /* Would handle TSC compressed images, standard formats, and transforms */
    LV_LOG_WARN("NeoChrom image drawing not fully implemented yet");
}

static void nema_wait_for_completion(nema_draw_unit_t * u)
{
    if (!u->gpu_busy) {
        return;
    }
    
#if LV_USE_OS
    /* Wait with OS synchronization */
    nema_cl_wait(&u->cl);
    lv_thread_sync_signal(&u->sync);
#else
    /* Blocking wait */
    nema_cl_wait(&u->cl);
#endif
    
    u->gpu_busy = false;
}

/**********************
 *   HAL IMPLEMENTATION
 **********************/

#if LV_USE_NEMA_HAL == LV_NEMA_HAL_STM32_DEFAULT

/* Simple HAL implementation for STM32 */
static void * nema_hal_malloc(size_t size)
{
    return lv_malloc(size);
}

static void nema_hal_free(void * ptr)
{
    lv_free(ptr);
}

/* NeoChrom HAL interface functions */
void * nema_host_malloc(size_t size)
{
    return nema_hal_malloc(size);
}

void nema_host_free(void * ptr)
{
    nema_hal_free(ptr);
}

#endif /* LV_USE_NEMA_HAL == LV_NEMA_HAL_STM32_DEFAULT */

#endif /* LV_USE_NEMA_GFX */

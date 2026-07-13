#include "drv/drv_setup.h"
#include "drv/drv_isr.h"
#include "drv/drv_config.h"
#include <stddef.h>

static ili9341_t *s_display = NULL;
static xpt2046_t *s_touch = NULL;

/* Internal ili9341.c accessor, not part of the public API. */
extern drv_spi_bus_t *ili9341_get_bus(ili9341_t *d);

#if defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
static void drv_hal_spi_tx_cplt_cb(SPI_HandleTypeDef *hspi)
{
    (void)hspi;
    DRV_ISR_DisplaySpiTxCplt();
}
#endif

drv_status_t DRV_Setup(void)
{
#if defined(DRV_PROVIDE_HAL_CALLBACKS) && (DRV_PROVIDE_HAL_CALLBACKS == 1)
    /* Keeps drv_isr_glue.o in the link; see the comment on the anchor. */
    extern void drv_isr_glue_anchor(void);
    drv_isr_glue_anchor();
#endif

    drv_spi_bus_t *disp_bus = drv_spi_bus_create((void *)DRV_DISP_SPI_HANDLE);
    if (disp_bus == NULL) {
        return DRV_ERR_HW;
    }
    drv_spi_bus_t *touch_bus = drv_spi_bus_create((void *)DRV_TOUCH_SPI_HANDLE);
    if (touch_bus == NULL) {
        return DRV_ERR_HW;
    }

    ili9341_config_t disp_cfg = {
        .bus = disp_bus,
        .cs = { (void *)DRV_DISP_CS_PORT, DRV_DISP_CS_PIN },
        .dc = { (void *)DRV_DISP_DC_PORT, DRV_DISP_DC_PIN },
        .reset = { (void *)DRV_DISP_RESET_PORT, DRV_DISP_RESET_PIN },
        .hor_res = DRV_DISP_HOR_RES,
        .ver_res = DRV_DISP_VER_RES,
        .rotation = DRV_DISP_ROTATION,
    };

    xpt2046_config_t touch_cfg = {
        .bus = touch_bus,
        .cs = { (void *)DRV_TOUCH_CS_PORT, DRV_TOUCH_CS_PIN },
        .irq = { (void *)DRV_TOUCH_IRQ_PORT, DRV_TOUCH_IRQ_PIN },
        .raw_x_min = DRV_TOUCH_RAW_X_MIN,
        .raw_x_max = DRV_TOUCH_RAW_X_MAX,
        .raw_y_min = DRV_TOUCH_RAW_Y_MIN,
        .raw_y_max = DRV_TOUCH_RAW_Y_MAX,
    };

    drv_status_t st = ILI9341_Init(&s_display, &disp_cfg);
    if (st != DRV_OK) {
        return st;
    }

    st = XPT2046_Init(&s_touch, &touch_cfg);
    if (st != DRV_OK) {
        return st;
    }

#if defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1)
    if (HAL_SPI_RegisterCallback((SPI_HandleTypeDef *)DRV_DISP_SPI_HANDLE,
                                  HAL_SPI_TX_COMPLETE_CB_ID,
                                  drv_hal_spi_tx_cplt_cb) != HAL_OK) {
        return DRV_ERR_HW;
    }
#endif

    return DRV_OK;
}

ili9341_t *DRV_GetDisplay(void)
{
    return s_display;
}

xpt2046_t *DRV_GetTouch(void)
{
    return s_touch;
}

void DRV_ISR_DisplaySpiTxCplt(void)
{
    if (s_display == NULL) {
        return;
    }
    drv_spi_tx_complete_isr(ili9341_get_bus(s_display));
}

void DRV_ISR_TouchPenDown(void)
{
    if (s_touch == NULL) {
        return;
    }
    XPT2046_PenIRQ_ISR(s_touch);
}

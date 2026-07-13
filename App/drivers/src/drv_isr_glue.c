/*
 * Default HAL callback wiring. Compiled only when the CMake option
 * DRV_PROVIDE_HAL_CALLBACKS=ON (default ON).
 *
 * If the app's SPI HAL is built with "Register Callbacks" enabled
 * (USE_HAL_SPI_REGISTER_CALLBACKS == 1, set by CubeMX -> Project Manager ->
 * Advanced Settings), DRV_Setup() registers its own HAL_SPI_TX_COMPLETE_CB_ID
 * callback directly (see drv_setup.c) and this file defines nothing global.
 *
 * Otherwise this file defines HAL_SPI_TxCpltCallback and
 * HAL_GPIO_EXTI_Callback, filtered to the display SPI handle / touch IRQ pin
 * and forwarding to the DRV_ISR_* entry points, falling through silently for
 * any other handle/pin so the app can extend them.
 *
 * An app that defines its own HAL_GPIO_EXTI_Callback (or HAL_SPI_TxCpltCallback,
 * without register-callbacks) must set DRV_PROVIDE_HAL_CALLBACKS=OFF and
 * forward to DRV_ISR_DisplaySpiTxCplt()/DRV_ISR_TouchPenDown() manually —
 * otherwise the two definitions collide at link time.
 */
#include "drv/drv_isr.h"
#include "drv/drv_config.h"

/* Called from DRV_Setup() purely to reference this object file. The weak HAL
 * callback overrides below are the only other symbols here, and a linker
 * never extracts a static-archive member just to override a weak definition —
 * without a strong referenced symbol this file silently drops out of the
 * link and the DMA tx-complete callback becomes the HAL's no-op default. */
void drv_isr_glue_anchor(void)
{
}

#if !(defined(USE_HAL_SPI_REGISTER_CALLBACKS) && (USE_HAL_SPI_REGISTER_CALLBACKS == 1))

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == (SPI_HandleTypeDef *)DRV_DISP_SPI_HANDLE) {
        DRV_ISR_DisplaySpiTxCplt();
    }
}

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
    if (pin == DRV_TOUCH_IRQ_PIN) {
        DRV_ISR_TouchPenDown();
    }
}

#endif /* !USE_HAL_SPI_REGISTER_CALLBACKS */

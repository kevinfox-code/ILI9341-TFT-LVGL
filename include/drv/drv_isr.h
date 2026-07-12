#ifndef DRV_ISR_H_
#define DRV_ISR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ISR entry points the application (or drv_isr_glue.c) must route to. Each is
 * a safe no-op if DRV_Setup() has not yet run.
 */
void DRV_ISR_DisplaySpiTxCplt(void);   /* from HAL_SPI_TxCpltCallback for display SPI  */
void DRV_ISR_TouchPenDown(void);       /* from EXTI callback for DRV_TOUCH_IRQ_PIN     */

#ifdef __cplusplus
}
#endif
#endif /* DRV_ISR_H_ */

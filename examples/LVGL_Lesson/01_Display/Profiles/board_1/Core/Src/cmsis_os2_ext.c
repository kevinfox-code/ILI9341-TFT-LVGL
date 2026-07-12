#include "cmsis_os2.h"

/* 
 * FreeRTOS CMSIS-RTOS2 wrapper provided by STM32CubeMX/FreeRTOS 
 * does not implement osThreadDetach. 
 * Since FreeRTOS tasks are deleted directly via osThreadTerminate (vTaskDelete),
 * explicit detaching is not strictly required for resource reclamation 
 * in this specific port/configuration, or it's a no-op.
 * We provide a dummy implementation to satisfy the linker.
 */
osStatus_t osThreadDetach (osThreadId_t thread_id) {
  (void)thread_id;
  return osOK;
}

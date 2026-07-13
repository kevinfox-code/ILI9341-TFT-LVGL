#ifndef APP_MAIN_H_
#define APP_MAIN_H_

/* Called from Core/Src/freertos.c's MX_FREERTOS_Init() `USER CODE BEGIN
 * RTOS_THREADS` block. Creates the TouchGFX task (which runs DRV_Setup()
 * then enters the TouchGFX main loop), the ~60Hz VSYNC timer, and this
 * example's demo tasks (RTC read/UART echo). */
void App_CreateTasks(void);

#endif /* APP_MAIN_H_ */

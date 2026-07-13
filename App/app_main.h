#ifndef APP_MAIN_H_
#define APP_MAIN_H_

/* Called from Core/Src/main.c's `USER CODE BEGIN 2` block, before
 * osKernelStart(). Brings up LVGL, the display/touch drivers, and the
 * initial screen. */
void App_Init(void);

/* Called from Core/Src/freertos.c's MX_FREERTOS_Init() `USER CODE BEGIN
 * RTOS_THREADS` block. Creates the GUI task, the 1ms LVGL tick timer, and
 * this example's demo tasks (RTC read/UART echo/clock display). */
void App_CreateTasks(void);

#endif /* APP_MAIN_H_ */

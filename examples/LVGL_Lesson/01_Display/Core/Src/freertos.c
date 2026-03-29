/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "misc/lv_timer.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx_hal_gpio.h"
#include "rtc.h"
#include "usart.h"
#include "lvgl.h"
#include "TouchController.h"
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
volatile uint32_t debugCounter = 0;

/* USER CODE END Variables */
/* Definitions for normalTask */
osThreadId_t normalTaskHandle;
const osThreadAttr_t normalTask_attributes = {
  .name = "normalTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for lvglTask */
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
  .name = "lvglTask",
  .stack_size = 2048 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for belowNormalTask */
osThreadId_t belowNormalTaskHandle;
const osThreadAttr_t belowNormalTask_attributes = {
  .name = "belowNormalTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for sharedQueue */
osMessageQueueId_t sharedQueueHandle;
const osMessageQueueAttr_t sharedQueue_attributes = {
  .name = "sharedQueue"
};
/* Definitions for lvglTimer */
osTimerId_t lvglTimerHandle;
const osTimerAttr_t lvglTimer_attributes = {
  .name = "lvglTimer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
osThreadId_t readAndLoadTimeHandle;
const osThreadAttr_t readAndLoadTime_attributes = {
  .name = "readAndLoadTime",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow3,
};

osThreadId_t dmaUartHandle;
const osThreadAttr_t dmaUart_attributes = {
  .name = "dmaUart",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow2,
};

osThreadId_t updateTimeGuiHandle;
const osThreadAttr_t updateTimeGui_attributes = {
  .name = "updateTimeGui",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow7,
};

void StartReadAndLoadTime(void *argument);
void StartDmaUartSender(void *argument);
void StartUpdateTimeGui(void *argument);

/* USER CODE END FunctionPrototypes */

void startNormalTask(void *argument);
void startLvglTask(void *argument);
void startBelowNormal(void *argument);
void lvglTimerCallback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of lvglTimer */
  lvglTimerHandle = osTimerNew(lvglTimerCallback, osTimerPeriodic, NULL, &lvglTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  osTimerStart(lvglTimerHandle, 1);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sharedQueue */
  sharedQueueHandle = osMessageQueueNew (16, sizeof(uint32_t), &sharedQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of normalTask */
  normalTaskHandle = osThreadNew(startNormalTask, NULL, &normalTask_attributes);

  /* creation of lvglTask */
  lvglTaskHandle = osThreadNew(startLvglTask, NULL, &lvglTask_attributes);

  /* creation of belowNormalTask */
  belowNormalTaskHandle = osThreadNew(startBelowNormal, NULL, &belowNormalTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  readAndLoadTimeHandle = osThreadNew(StartReadAndLoadTime, NULL, &readAndLoadTime_attributes);
  dmaUartHandle = osThreadNew(StartDmaUartSender, NULL, &dmaUart_attributes);
  updateTimeGuiHandle = osThreadNew(StartUpdateTimeGui, NULL, &updateTimeGui_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_startNormalTask */
/**
  * @brief  Function implementing the normalTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_startNormalTask */
void startNormalTask(void *argument)
{
  /* USER CODE BEGIN startNormalTask */
  /* Nothing to do — suspend indefinitely rather than burning CPU/context switches */
  for(;;)
  {
    osDelay(osWaitForever);
  }
  /* USER CODE END startNormalTask */
}

/* USER CODE BEGIN Header_startLvglTask */
/**
* @brief Function implementing the lvglTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_startLvglTask */
void startLvglTask(void *argument)
{
  /* USER CODE BEGIN startLvglTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(5);
    TouchController_Poll();
    lv_timer_handler(); /* let LVGL handle tasks */
  }
  /* USER CODE END startLvglTask */
}

/* USER CODE BEGIN Header_startBelowNormal */
/**
* @brief Function implementing the belowNormalTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_startBelowNormal */
void startBelowNormal(void *argument)
{
  /* USER CODE BEGIN startBelowNormal */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);

    HAL_GPIO_TogglePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin);

    debugCounter++;

    printf("Debug Counter: %" PRIu32 "\n", debugCounter);
  }
  /* USER CODE END startBelowNormal */
}

/* lvglTimerCallback function */
void lvglTimerCallback(void *argument)
{
  /* USER CODE BEGIN lvglTimerCallback */
  lv_tick_inc(1);

  /* USER CODE END lvglTimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void StartReadAndLoadTime(void *argument)
{
  /* Infinite loop */
  for(;;)
  {
    /* Read RTC time once per second and push seconds-since-midnight (fits in 32-bit).
       Adjust delay or packing format if consumers expect a different layout. */
    osDelay(1000);

    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    uint32_t message = (uint32_t)(sTime.Hours * 3600U + sTime.Minutes * 60U + sTime.Seconds);
    osMessageQueuePut(sharedQueueHandle, &message, 0, 0);
  }
}

void StartDmaUartSender(void *argument)
{
  /* Infinite loop */
  for(;;)
  {
    uint32_t message = 0;
    if (osMessageQueueGet(sharedQueueHandle, &message, NULL, osWaitForever) == osOK)
    {
      uint32_t hours = message / 3600U;
      uint32_t minutes = (message % 3600U) / 60U;
      uint32_t secs = message % 60U;

      char buf[32];
      int len = snprintf(buf, sizeof(buf), "RTC %02u:%02u:%02u\r\n", (unsigned)hours, (unsigned)minutes, (unsigned)secs);

      /* Wait until UART is ready for new DMA transfer */
      while (HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY)
      {
        osDelay(1);
      }

      HAL_UART_Transmit_DMA(&huart2, (uint8_t*)buf, (uint16_t)len);

      /* Wait for transmission to complete before handling next message */
      while (HAL_UART_GetState(&huart2) != HAL_UART_STATE_READY)
      {
        osDelay(1);
      }
    }
  }
}

void StartUpdateTimeGui(void *argument)
{
  extern lv_obj_t * time_label;
  extern lv_obj_t * date_label;
  
  /* Infinite loop */
  for(;;)
  {
    osDelay(1000);
    
    if (time_label != NULL && date_label != NULL)
    {
      RTC_TimeTypeDef sTime;
      RTC_DateTypeDef sDate;
      HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
      HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

      char time_str[16];
      snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);

      char date_str[16];
      snprintf(date_str, sizeof(date_str), "%02d/%02d/20%02d", sDate.Month, sDate.Date, sDate.Year);

      /* LVGL is not thread-safe — acquire the lock before touching any LVGL objects */
      lv_lock();
      lv_label_set_text(time_label, time_str);
      lv_label_set_text(date_label, date_str);
      lv_unlock();
    }
  }
}

/* USER CODE END Application */


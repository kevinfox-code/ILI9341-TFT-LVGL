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
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
  .stack_size = 128 * 4,
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
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of sharedQueue */
  sharedQueueHandle = osMessageQueueNew (16, sizeof(uint16_t), &sharedQueue_attributes);

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
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
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
    osDelay(1);
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
    osDelay(1);
  }
  /* USER CODE END startBelowNormal */
}

/* lvglTimerCallback function */
void lvglTimerCallback(void *argument)
{
  /* USER CODE BEGIN lvglTimerCallback */

  /* USER CODE END lvglTimerCallback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */


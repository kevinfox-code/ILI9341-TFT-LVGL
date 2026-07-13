/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

extern "C" {
#include "drv/drv_setup.h"
#include "drv_constants.h"
}

void STM32TouchController::init()
{
    /* Nothing to do: DRV_Setup() (run at the top of the TouchGFX task,
     * App/app_main.c) initializes the XPT2046 before the first frame. */
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    xpt2046_t *touch = DRV_GetTouch();
    ili9341_t *disp = DRV_GetDisplay();
    if (touch == nullptr || disp == nullptr)
    {
        return false;
    }

    uint16_t hor_res, ver_res;
    ILI9341_GetResolution(disp, &hor_res, &ver_res);

    uint16_t px, py;
    if (!XPT2046_ReadPoint(touch, &px, &py, hor_res, ver_res, DRV_DISP_ROTATION))
    {
        return false;
    }
    x = px;
    y = py;
    return true;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

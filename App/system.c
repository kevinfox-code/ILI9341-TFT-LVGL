#include "system.h"
#include "drv_constants.h"
#include "cmsis_os2.h"
#include <stdio.h>

uint32_t System_GetSecondsSinceMidnight(void)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(APP_RTC_HANDLE, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(APP_RTC_HANDLE, &sDate, RTC_FORMAT_BIN);
    return (uint32_t)(sTime.Hours * 3600U + sTime.Minutes * 60U + sTime.Seconds);
}

void System_GetClockStrings(char *time_buf, size_t time_buf_len, char *date_buf, size_t date_buf_len)
{
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    HAL_RTC_GetTime(APP_RTC_HANDLE, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(APP_RTC_HANDLE, &sDate, RTC_FORMAT_BIN);

    if (time_buf != NULL) {
        snprintf(time_buf, time_buf_len, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
    }
    if (date_buf != NULL) {
        snprintf(date_buf, date_buf_len, "%02d/%02d/20%02d", sDate.Month, sDate.Date, sDate.Year);
    }
}

void System_ToggleDebugLed(void)
{
    HAL_GPIO_TogglePin(APP_DEBUG_LED_PORT, APP_DEBUG_LED_PIN);
}

void System_Fatal(void)
{
    Error_Handler();
}

void System_PetWatchdog(void)
{
    HAL_IWDG_Refresh(APP_IWDG_HANDLE);
}

bool System_UartSendLine(const char *data, uint16_t len)
{
    static uint8_t s_tx_buf[64];
    if (data == NULL || len > sizeof(s_tx_buf)) {
        return false;
    }
    for (uint16_t i = 0; i < len; i++) {
        s_tx_buf[i] = (uint8_t)data[i];
    }

    while (HAL_UART_GetState(APP_UART_HANDLE) != HAL_UART_STATE_READY) {
        osDelay(1);
    }
    HAL_UART_Transmit_DMA(APP_UART_HANDLE, s_tx_buf, len);
    while (HAL_UART_GetState(APP_UART_HANDLE) != HAL_UART_STATE_READY) {
        osDelay(1);
    }
    return true;
}

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

/* ---- UART comms --------------------------------------------------------
 * TX: DMA sends serialized by a mutex (broadcast task + time-sync responses
 * share the port). RX: byte-wise interrupt reception into a single-producer
 * (ISR) / single-consumer (task) ring buffer.
 */

static osMutexId_t s_uart_tx_mutex;
static const osMutexAttr_t k_uart_tx_mutex_attr = { .name = "uartTx" };

static volatile uint8_t s_rx_ring[128];
static volatile uint16_t s_rx_head; /* ISR writes */
static volatile uint16_t s_rx_tail; /* task reads */
static uint8_t s_rx_byte;

void System_CommsInit(void)
{
    s_uart_tx_mutex = osMutexNew(&k_uart_tx_mutex_attr);
}

bool System_UartSendLine(const char *data, uint16_t len)
{
    static uint8_t s_tx_buf[64];
    if (data == NULL || len > sizeof(s_tx_buf)) {
        return false;
    }

    osMutexAcquire(s_uart_tx_mutex, osWaitForever);
    for (uint16_t i = 0; i < len; i++) {
        s_tx_buf[i] = (uint8_t)data[i];
    }

    /* Poll gState (TX side) only — HAL_UART_GetState() ORs in RxState,
     * which is permanently BUSY_RX once interrupt reception is armed. */
    while (APP_UART_HANDLE->gState != HAL_UART_STATE_READY) {
        osDelay(1);
    }
    HAL_UART_Transmit_DMA(APP_UART_HANDLE, s_tx_buf, len);
    while (APP_UART_HANDLE->gState != HAL_UART_STATE_READY) {
        osDelay(1);
    }
    osMutexRelease(s_uart_tx_mutex);
    return true;
}

void System_UartStartReceive(void)
{
    HAL_UART_Receive_IT(APP_UART_HANDLE, &s_rx_byte, 1);
}

size_t System_UartReceive(uint8_t *buf, size_t maxlen)
{
    size_t n = 0;
    while (n < maxlen && s_rx_tail != s_rx_head) {
        buf[n++] = s_rx_ring[s_rx_tail];
        s_rx_tail = (uint16_t)((s_rx_tail + 1u) % sizeof(s_rx_ring));
    }
    return n;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart != APP_UART_HANDLE) {
        return;
    }
    uint16_t next = (uint16_t)((s_rx_head + 1u) % sizeof(s_rx_ring));
    if (next != s_rx_tail) { /* drop the byte if the ring is full */
        s_rx_ring[s_rx_head] = s_rx_byte;
        s_rx_head = next;
    }
    HAL_UART_Receive_IT(huart, &s_rx_byte, 1); /* re-arm */
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart != APP_UART_HANDLE) {
        return;
    }
    /* Overrun/framing/noise abort IT reception; clear and re-arm so a
     * glitch on the line can't permanently kill the time-sync channel. */
    __HAL_UART_CLEAR_OREFLAG(huart);
    HAL_UART_Receive_IT(huart, &s_rx_byte, 1);
}

/* ---- RTC date/time ------------------------------------------------------ */

bool System_GetDateTime(clock_datetime_t *dt)
{
    if (dt == NULL) {
        return false;
    }
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    /* GetDate must follow GetTime to unlock the shadow registers. */
    HAL_RTC_GetTime(APP_RTC_HANDLE, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(APP_RTC_HANDLE, &sDate, RTC_FORMAT_BIN);

    dt->year = (uint16_t)(2000u + sDate.Year);
    dt->month = sDate.Month;
    dt->day = sDate.Date;
    dt->hours = sTime.Hours;
    dt->minutes = sTime.Minutes;
    dt->seconds = sTime.Seconds;
    dt->weekday = sDate.WeekDay; /* HAL RTC_WEEKDAY_* is already ISO 1=Mon..7=Sun */
    return true;
}

bool System_SetDateTime(const clock_datetime_t *dt)
{
    if (dt == NULL || dt->year < 2000u || dt->year > 2099u) {
        return false;
    }
    RTC_TimeTypeDef sTime = { 0 };
    RTC_DateTypeDef sDate = { 0 };

    sTime.Hours = dt->hours;
    sTime.Minutes = dt->minutes;
    sTime.Seconds = dt->seconds;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    sDate.Year = (uint8_t)(dt->year - 2000u);
    sDate.Month = dt->month;
    sDate.Date = dt->day;
    sDate.WeekDay = (dt->weekday >= 1u && dt->weekday <= 7u)
                        ? dt->weekday
                        : clock_weekday(dt->year, dt->month, dt->day);

    /* Date first, then time: HAL_RTC_SetTime resynchronizes the shadow
     * registers, so finishing with it leaves a consistent snapshot. */
    if (HAL_RTC_SetDate(APP_RTC_HANDLE, &sDate, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }
    if (HAL_RTC_SetTime(APP_RTC_HANDLE, &sTime, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }
    /* Time is now host-set: make sure the first-boot seed in MX_RTC_Init()
     * never clobbers it after a reset. */
    HAL_RTCEx_BKUPWrite(APP_RTC_HANDLE, RTC_BKP_DR0, RTC_INIT_MAGIC);
    return true;
}

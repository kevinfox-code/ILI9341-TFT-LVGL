#ifndef SYSTEM_H_
#define SYSTEM_H_
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "clock_protocol.h" /* clock_datetime_t (HAL-free) */

/*
 * SystemInit-style glue layer: the only App/ file (besides drv_constants.h
 * itself) that may touch CubeMX-generated handles. Everything else in App/
 * — app_main.c, gui/ — calls these System_* functions instead of HAL_*
 * directly, so the demo tasks stay portable across Profiles/<board>.
 */

/* Seconds since midnight, read from the RTC. */
uint32_t System_GetSecondsSinceMidnight(void);

/* Formats "HH:MM:SS" / "MM/DD/YYYY" into caller-owned buffers (>=9 / >=11
 * bytes respectively) from the current RTC time/date. */
void System_GetClockStrings(char *time_buf, size_t time_buf_len, char *date_buf, size_t date_buf_len);

void System_ToggleDebugLed(void);

/* Refreshes the independent watchdog. Must be called often enough to beat
 * its reload timeout (see IWDG_Init in iwdg.c) or the MCU resets. */
void System_PetWatchdog(void);

/* Wraps the CubeMX-generated Error_Handler() so callers never need main.h. */
void System_Fatal(void);

/* Creates the comms primitives (UART TX mutex). Call once from
 * App_CreateTasks(), after the kernel objects can be created and before any
 * task uses System_UartSendLine/System_UartReceive. */
void System_CommsInit(void);

/* Blocking (from the calling task's perspective — internally polls via
 * osDelay) UART send over DMA. Thread-safe (serialized by an internal
 * mutex). Returns false if len exceeds the internal staging buffer. */
bool System_UartSendLine(const char *data, uint16_t len);

/* Arms interrupt-driven UART reception into an internal ring buffer.
 * Call once from the task that will consume received bytes. */
void System_UartStartReceive(void);

/* Drains up to maxlen received bytes into buf; returns the count (0 if
 * none pending). Single-consumer: call from one task only. */
size_t System_UartReceive(uint8_t *buf, size_t maxlen);

/* Reads the current RTC date+time (BIN format, year 2000-based). Returns
 * false only on invalid args. */
bool System_GetDateTime(clock_datetime_t *dt);

/* Writes date+time to the battery-backed RTC and stamps the backup
 * register so the CubeMX seed logic won't overwrite it on reboot. The
 * caller is expected to pass an already-validated clock_datetime_t (see
 * clock_datetime_valid()); returns false on invalid args or HAL error. */
bool System_SetDateTime(const clock_datetime_t *dt);

#endif /* SYSTEM_H_ */

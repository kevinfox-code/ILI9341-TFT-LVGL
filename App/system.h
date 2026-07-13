#ifndef SYSTEM_H_
#define SYSTEM_H_
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

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

/* Wraps the CubeMX-generated Error_Handler() so callers never need main.h. */
void System_Fatal(void);

/* Blocking (from the calling task's perspective — internally polls via
 * osDelay) UART send over DMA. Returns false if len exceeds the internal
 * staging buffer. */
bool System_UartSendLine(const char *data, uint16_t len);

#endif /* SYSTEM_H_ */

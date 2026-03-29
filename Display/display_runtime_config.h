#ifndef DISPLAY_RUNTIME_CONFIG_H_
#define DISPLAY_RUNTIME_CONFIG_H_

/*
 * Runtime policy knobs for display and touch drivers.
 *
 * All values can be overridden from CMake with target_compile_definitions.
 */

#ifndef DISPLAY_DETERMINISTIC_MODE
#define DISPLAY_DETERMINISTIC_MODE 1
#endif

/* Timeout used for blocking SPI transactions in milliseconds. */
#ifndef DISPLAY_SPI_TIMEOUT_MS
#define DISPLAY_SPI_TIMEOUT_MS 100U
#endif

/*
 * If enabled, LVGL flush is completed immediately when a previous DMA transfer
 * is still in-flight. This bounds worst-case latency at the cost of dropped frames.
 */
#ifndef DISPLAY_DROP_FRAME_IF_DMA_BUSY
#define DISPLAY_DROP_FRAME_IF_DMA_BUSY DISPLAY_DETERMINISTIC_MODE
#endif

/* Wait timeout for DMA availability when DISPLAY_DROP_FRAME_IF_DMA_BUSY is disabled. */
#ifndef DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS
#define DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS 20U
#endif

/* Delay granularity while waiting for DMA availability. */
#ifndef DISPLAY_DMA_BUSY_WAIT_SLICE_MS
#define DISPLAY_DMA_BUSY_WAIT_SLICE_MS 1U
#endif

#endif /* DISPLAY_RUNTIME_CONFIG_H_ */

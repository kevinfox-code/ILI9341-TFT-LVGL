# ILI9341 + LVGL Driver — Plan and Progress

This document is the single source of record for architecture decisions, modularization
phases, memory configuration, test infrastructure, and code review findings for the
01_Display example project.

---

## Architecture Overview

```
Application (main.c, freertos.c)
    ↓
LVGL Port Adapters (LCDController.c, TouchController.c)
    ↓
Hardware Drivers (ILI9341.c, XPT2046.c)
    ↓
Board Abstraction (board_drivers.c, board_config.h)
    ↓
HAL / Peripherals (SPI1, SPI2, GPIO, DMA2, RTC, UART2)
```

### Key design decisions

- **Config structs** (`ILI9341_Config_t`, `XPT2046_Config_t`) decouple drivers from platform.
- **Board adapter** is the single source of truth for GPIO/SPI mappings; board selection is
  purely compile-time.
- **Deterministic policy** controls whether the display flush path drops a frame or waits
  when a DMA transfer is still in flight.
- **DMA-backed flush** keeps the CPU unblocked between render and transmit.
- **Touch calibration** supports runtime override with board defaults as fallback.
- **LVGL runs in one task**; all LVGL API calls from other tasks require `lv_lock()`/`lv_unlock()`.

---

## Phase 1 & 2 — Board Abstraction Layer (2026-03-29)

### Completed

- Added single board mapping header: `App/Board/board_config.h`
- Added board adapter module: `App/Board/board_drivers.h` / `board_drivers.c`
- Integrated board adapter into startup path: `Core/Src/main.c`
- Integrated board adapter into LVGL display path: `Drivers/Display/LCDController.c`
- Integrated board calibration and IRQ mapping into touch path:
  `Drivers/Display/TouchController.c`
- Wired touch polling into LVGL task loop: `Core/Src/freertos.c`
- Added board module to build system: `CMakeLists.txt`

### Solutions applied

- Centralized HAL resource wiring (SPI handles and GPIO mappings) into `board_config.h`.
- Kept low-level driver contracts stable (`ILI9341_Config_t`, `XPT2046_Config_t`) to avoid
  broad churn.
- Added adapter getters to provide immutable driver config objects to callers.
- Replaced hardcoded display resolution usage in LVGL display port with board-driven values.
- Added board-managed touch calibration model:
  - compile-time defaults from board config
  - runtime override via `Board_SetTouchCalibration()`
  - reset path via `Board_ResetTouchCalibration()`
- Updated touch calibration routine to compute min/max from collected points and store
  runtime override.

### Findings

- Existing architecture was already close to modular; portability blockers were concentrated
  in LVGL glue and startup wiring, not in core SPI transaction logic.
- Touch support was partially wired before this phase; startup init and periodic poll path
  are now explicit and board-agnostic.

### Validation

- CMake build completed successfully after modularization changes.
- Static analysis reported no errors in modified source files.
- Display and touch initialization are now both driven from board config getter APIs.

---

## Phase 3 — Multi-Board Selection (2026-03-29)

### Completed

- Added per-board mapping headers:
  - `App/Board/boards/board_stm32f446.h`
  - `App/Board/boards/board_stm32f411.h`
- Updated board selector and contract gate: `App/Board/board_config.h`
- Added CMake board selector scaffold with STM32F446 as default: `CMakeLists.txt`

### Solutions applied

- Board selection uses compile definitions: `BOARD_STM32F446` / `BOARD_STM32F411`
- CMake cache variable `BOARD_TARGET` controls which define is emitted.
- `board_config.h` enforces contract at compile time:
  - required mapping macros for display and touch resources
  - display resolution non-zero check
  - display rotation range check (0..3)
  - touch calibration min/max ordering checks

### Porting workflow

1. Duplicate an existing board header under `App/Board/boards/` and update mappings.
2. Add new board option in `CMakeLists.txt` `BOARD_TARGET` mapping.
3. Ensure `board_config.h` selects the new board header.
4. Build with `-D BOARD_TARGET=your_board`.

### Remaining improvements

- Add optional non-volatile storage persistence for runtime touch calibration override.

---

## Phase 4 — Display Stack Consolidation and System Tests (2026-03-29)

### Completed

- Consolidated display and touch control sources into `Drivers/Display/`.
- Updated build paths; removed legacy empty folders `Drivers/ILI9341` and
  `Drivers/XPT2046`.
- Created full-system test scaffold:
  - `tests/full_system/validate_display_stack.py` — structural validation
  - `tests/full_system/hardware_smoke_checklist.md` — hardware run checklist

### Validation

Run structural validation from project root:

```sh
python3 tests/full_system/validate_display_stack.py
```

A non-zero exit code means one or more required checks failed.

---

## Phase 5 — Deterministic Runtime Policy (2026-03-29)

### Completed

- Added `Drivers/Display/display_runtime_config.h` with all policy knobs.
- Updated display transfer code to use bounded SPI timeouts throughout.
- Updated LVGL flush handling with two selectable policies:
  - **Drop-frame** (default): if DMA is busy, signal `flush_ready` immediately and skip
    this frame — guarantees bounded latency.
  - **Bounded wait**: poll `ili_dma_busy` up to `DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS` before
    dropping.

### CMake policy knobs

| Define | Default | Effect |
|--------|---------|--------|
| `DISPLAY_DETERMINISTIC_MODE` | `ON` | Master switch; enables drop-frame policy |
| `DISPLAY_DROP_FRAME_IF_DMA_BUSY` | `ON` | Drop frame if DMA busy instead of waiting |
| `DISPLAY_SPI_TIMEOUT_MS` | `100` | Blocking SPI transaction timeout (ms) |
| `DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS` | `20` | Max DMA poll wait time (ms) |
| `DISPLAY_DMA_BUSY_WAIT_SLICE_MS` | `1` | Poll granularity (ms) |

Example configure command:

```bash
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release \
  -D BOARD_TARGET=stm32f446 \
  -D DISPLAY_DETERMINISTIC_MODE=ON \
  -D DISPLAY_DROP_FRAME_IF_DMA_BUSY=ON \
  -D DISPLAY_SPI_TIMEOUT_MS=100
```

---

## Phase 6 — Code Review Fixes (2026-03-29)

Full code review was performed across the display driver stack. All identified issues were
fixed in the same session.

### Critical fixes

**Double `lv_display_flush_ready` on DMA timeout** (`LCDController.c`)
When the bounded-wait path timed out, it called `lv_display_flush_ready` and returned.
The stalled DMA ISR then also called `lv_display_flush_ready` when it eventually completed.
Calling this twice is undefined behavior in LVGL and corrupts display state.
Fix: added `ILI9341_CancelFlushCallback()` which nulls the `dma_display` pointer so the
ISR call is suppressed when the port layer has already signalled ready.

**LVGL API called from non-LVGL task** (`freertos.c`)
`StartUpdateTimeGui` called `lv_label_set_text` directly from its own task. LVGL is not
thread-safe; concurrent access from multiple tasks corrupts internal state.
Fix: wrapped both `lv_label_set_text` calls with `lv_lock()` / `lv_unlock()`.

### All fixes applied

| # | File | Fix |
|---|------|-----|
| 1 | `ILI9341.h` | Removed duplicate `ILI9341_SetRotation` declaration |
| 2 | `ILI9341.c` | Removed redundant `0x2C` command from `DrawBitmap` |
| 3 | `ILI9341.h`, `board_drivers.c` | Added `hor_res`/`ver_res` to config struct; `FillScreen` uses them |
| 4 | `ILI9341.c` | Added `ILI9341_CancelFlushCallback()` to suppress ISR flush_ready |
| 5 | `LCDController.c` | DMA timeout path calls `ILI9341_CancelFlushCallback()` before flush_ready |
| 6 | `ILI9341.c` | Removed hardcoded `SetRotation(3)` from `Init`; rotation set by port layer only |
| 7 | `LCDController.c` | Removed empty `disp_init` stub and its call |
| 8 | `freertos.c` | Wrapped `lv_label_set_text` calls with `lv_lock()` / `lv_unlock()` |
| 9 | `freertos.c` | `normalTask` suspends with `osWaitForever` instead of waking every 1 ms |
| 10 | `TouchController.c` | Renamed inner `cal` struct to `cal_data`; removed misleading `main_screen` variable |
| 11 | `XPT2046.c` | X and Y both use discard-first, then average-two reads |
| 12 | `LCDController.c` | Added `static` to `disp_flush_enabled` |
| 13 | `TouchController.c` | Removed local `__io_putchar`; `syscalls.c` provides it via `weak` symbol |

---

## Memory Configuration

### Hardware

| Resource | Size |
|----------|------|
| RAM | 128 KB |
| Flash | 512 KB |

### FreeRTOS (`Core/Inc/FreeRTOSConfig.h`)

```c
#define configTOTAL_HEAP_SIZE    ((size_t)(32 * 1024))  // 32 KB
#define configMINIMAL_STACK_SIZE ((uint16_t)128)         // 512 bytes
#define configENABLE_FPU         1
#define configTICK_RATE_HZ       ((TickType_t)1000)
```

### LVGL (`Drivers/lv_conf.h`)

```c
#define LV_MEM_SIZE    (16 * 1024U)          // 16 KB LVGL heap
#define LV_COLOR_DEPTH 16                    // RGB565
#define LV_USE_OS      LV_OS_CMSIS_RTOS2
```

### Linker (`STM32F446XX_FLASH.ld`)

```
_Min_Heap_Size  = 0x200;   // 512 bytes
_Min_Stack_Size = 0x1000;  // 4 KB MSP stack
```

### RAM usage summary

| Component | Size | Source |
|-----------|------|--------|
| FreeRTOS heap | 32 KB | `configTOTAL_HEAP_SIZE` |
| LVGL heap | 16 KB | `LV_MEM_SIZE` |
| Display buffers (2×) | 12.8 KB | Static in `LCDController.c` |
| LVGL task stack | 8 KB | `lvglTask_attributes.stack_size` |
| Main stack (MSP) | 4 KB | `_Min_Stack_Size` |
| Other task stacks | ~3 KB | All other tasks |
| **Total** | **~76 KB** | **Fits in 128 KB** |

### Task inventory

| Task | Stack | Priority | Purpose |
|------|-------|----------|---------|
| `lvglTask` | 8 KB | `osPriorityLow` | LVGL timer handler; owns all LVGL API calls |
| `readAndLoadTime` | 1 KB | `osPriorityLow3` | Reads RTC, pushes to queue |
| `dmaUart` | 1 KB | `osPriorityLow2` | Formats and sends RTC data via UART2 DMA |
| `updateTimeGui` | 1 KB | `osPriorityLow7` | Updates time/date labels (uses `lv_lock`) |
| `normalTask` | 512 B | `osPriorityNormal` | Placeholder; suspends indefinitely |
| `belowNormalTask` | 512 B | `osPriorityBelowNormal` | LED blink, debug counter |

### Troubleshooting memory

- **"region RAM overflowed"** at link time: reduce `configTOTAL_HEAP_SIZE` or `LV_MEM_SIZE`, or shrink display buffers.
- **Stack overflow** at runtime: increase task `stack_size` or enable `configCHECK_FOR_STACK_OVERFLOW`.
- **LVGL out of memory**: increase `LV_MEM_SIZE` or simplify the GUI widget tree.
- **Build > 128 KB RAM**: audit `.bss` section; consider external SRAM.

---

## Hardware Smoke Checklist

Use after flashing to validate the full integrated display subsystem.

### Preconditions

- Firmware built from current `main` branch.
- Board mapping selected correctly via `BOARD_TARGET`.
- Display and touch hardware connected to pins defined in `App/Board/boards/`.

### Checks

1. **Boot** — device boots without HardFault; no repeated init/reset loop.
2. **Display output** — backlight powers on; LVGL content renders with correct orientation;
   no persistent tearing or corrupted frame regions.
3. **Touch input** — press detected; drag tracks; corner taps map to expected UI locations.
4. **Calibration** — run `touch_calibrate()`; verify improved accuracy; verify
   `Board_ResetTouchCalibration()` restores defaults.
5. **Stability** — leave GUI running 10+ minutes; no freeze, watchdog reset, or DMA deadlock.

### Result

Pass if all checks complete without failure. On failure, capture UART traces and note the
board target used.

---

## Board Contract Quick Reference

Every board header under `App/Board/boards/` must define:

```
BOARD_DISP_SPI_HANDLE       BOARD_TOUCH_SPI_HANDLE
BOARD_DISP_CS_PORT/PIN      BOARD_TOUCH_CS_PORT/PIN
BOARD_DISP_DC_PORT/PIN      BOARD_TOUCH_IRQ_PORT/PIN
BOARD_DISP_RESET_PORT/PIN
BOARD_DISP_HOR_RES          BOARD_TOUCH_RAW_X_MIN/MAX
BOARD_DISP_VER_RES          BOARD_TOUCH_RAW_Y_MIN/MAX
BOARD_DISP_ROTATION (0..3)
```

`board_config.h` enforces all of these at compile time and will produce a descriptive
`#error` for any missing or invalid value.

# Release Notes — v1.0.0

**ILI9341 TFT + XPT2046 Touch Driver for STM32 + LVGL**

---

## Overview

v1.0.0 is the first structured release of this driver package. The primary goal of this
release was to bring the driver up to production quality and restructure the repository so
users can add a single folder to their `Drivers/` directory and be running in minutes.

---

## What's New

### Repository restructured as a drop-in driver package

The entire repo is now the driver package. Add it to any CubeMX-generated project by
cloning or copying it into your `Drivers/` folder:

```bash
git submodule add <url> Drivers/ILI9341-TFT-LVGL
```

Driver source files now live at the repo root in two clean directories:

```
Display/    ILI9341, LCDController, XPT2046, TouchController, display_runtime_config
Board/      board_config, board_drivers, boards/board_stm32f446, boards/board_stm32f411
```

### Board abstraction layer

A new `Board/` module decouples hardware pin assignments from driver logic. All SPI
handles, GPIO ports/pins, display resolution, rotation, and touch calibration defaults are
defined in a single board header that maps directly to CubeMX-generated symbols.

- `board_config.h` — compile-time contract: missing or invalid `BOARD_*` macros are
  caught as `#error` before linking.
- `board_drivers.h / .c` — typed getters (`Board_GetDisplayConfig()`,
  `Board_GetTouchConfig()`, etc.) provide clean access to board resources.
- Runtime touch calibration override via `Board_SetTouchCalibration()` /
  `Board_ResetTouchCalibration()`.

**Board targets:** `stm32f446` (tested), `stm32f411` (template provided).

### DMA-backed display flush with deterministic timing policy

The display flush path (`LCDController.c`) uses SPI DMA for non-blocking frame transfers.
A compile-time policy (`display_runtime_config.h`) controls behavior when a DMA transfer
is still in-flight when LVGL requests the next flush:

| Policy                           | Behavior                                               |
| -------------------------------- | ------------------------------------------------------ |
| `DISPLAY_DROP_FRAME_IF_DMA_BUSY` | Drop the frame immediately — zero blocking, bounded latency |
| DMA busy-wait (timeout)          | Wait up to `DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS` ms, then give up |

All knobs default to deterministic (drop-frame) mode and can be overridden from CMake
without touching driver source.

### 4-point touch calibration

`touch_calibrate()` displays green crosshairs at the four screen corners and walks the
user through a guided tap sequence. Raw ADC values are printed via `printf` so you can
update `BOARD_TOUCH_RAW_*` defaults in your board header. Runtime calibration persists via
`Board_SetTouchCalibration()` without requiring a reflash.

### Comprehensive from-scratch setup guide

The root `README.md` is a single, self-contained 13-step guide covering:

- Clock and SPI configuration in CubeMX (with exact register values for STM32F446)
- DMA setup for non-blocking display transfers
- FreeRTOS heap sizing and `FreeRTOSConfig.h` key values
- LVGL v9 `lv_conf.h` required settings
- CMake integration snippets ready to paste
- LVGL task structure and thread-safety (`lv_lock` / `lv_unlock`)
- Hardware smoke checklist for first-boot verification

---

## Compatibility

| Component        | Version / Part        |
| ---------------- | --------------------- |
| MCU (tested)     | STM32F446RE (Nucleo-64) |
| MCU (template)   | STM32F411             |
| Display IC       | ILI9341 (2.8" SPI TFT, 320×240) |
| Touch IC         | XPT2046               |
| LVGL             | v9.2.2                |
| HAL              | STM32 HAL (CubeMX-generated) |
| RTOS             | FreeRTOS via CMSIS-RTOS V2 |
| Build system     | CMake 3.22+           |
| C standard       | C11                   |

---

## Breaking Changes

This is the first versioned release. Projects using the earlier unstructured layout
(`src/ILI9341.c`, `src/LCDController.c`, etc.) will need to update their CMake include
paths and add the board header for their target MCU.

---

## Known Limitations

- Touch calibration values are not persisted across power cycles (no NVM/flash write). The
  four `BOARD_TOUCH_RAW_*` defaults in the board header are the permanent baseline.
- Only portrait and landscape rotations are calibrated and tested; flipped modes (1 and 2)
  may require touch axis remapping adjustments.
- `XPT2046_TouchDetected()` requires a wired PENIRQ GPIO. IRQ-less polling is not
  supported without modifying the driver.

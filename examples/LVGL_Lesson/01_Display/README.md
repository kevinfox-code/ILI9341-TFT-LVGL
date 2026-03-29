# 01_Display — Reference Example

Working STM32F446RE project demonstrating the ILI9341 + XPT2046 driver stack with LVGL v9
and FreeRTOS. See the [root README](../../../README.md) for the full setup guide.

## Project Layout

```text
01_Display/
├── Core/
│   ├── Inc/
│   │   ├── FreeRTOSConfig.h
│   │   ├── main.h                  # CubeMX GPIO/SPI symbol definitions
│   │   └── spi.h
│   └── Src/
│       ├── main.c                  # Peripheral init + lv_init() + load_gui()
│       ├── freertos.c              # LVGL tick timer + lvglTask
│       └── cmsis_os2_ext.c
├── Drivers/
│   └── lv_conf.h                   # LVGL configuration
├── CMakeLists.txt                   # References Display/ and Board/ at repo root
└── cmake/stm32cubemx/CMakeLists.txt # CubeMX-generated sources
```

The driver source files (`Display/`, `Board/`) live at the repository root and are
referenced via relative paths in `CMakeLists.txt`.

## Build

```bash
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -D BOARD_TARGET=stm32f446
cmake --build build
```

## Board Selection

```bash
cmake -S . -B build -D BOARD_TARGET=stm32f411
```

See [Porting to a New Board](../../../README.md#porting-to-a-new-board) in the root README.

## Deterministic Display Policy

```bash
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release \
  -D DISPLAY_DETERMINISTIC_MODE=ON \
  -D DISPLAY_DROP_FRAME_IF_DMA_BUSY=ON
```

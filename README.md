# ILI9341 TFT + XPT2046 Touch Driver for STM32 + LVGL

Lightweight display and touch drivers for the 2.8" ILI9341 SPI TFT with XPT2046
resistive touchscreen, fully integrated with LVGL v9 and FreeRTOS on STM32F4.

**Tested on:** STM32F446RE (Nucleo-64)
**LVGL version:** 9.2.2
**HAL:** STM32 HAL (CubeMX-generated)

---

## Installation

Clone or download this repository and place it inside your project's `Drivers/` folder:

```bash
# As a git submodule (recommended)
git submodule add <repo-url> Drivers/ILI9341-TFT-LVGL

# Or copy manually
cp -r ILI9341-TFT-LVGL/ your_project/Drivers/
```

After adding LVGL (`Drivers/lvgl/`) and generating your CubeMX project, add the following
to your `CMakeLists.txt`:

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    Drivers/ILI9341-TFT-LVGL/Board/board_drivers.c
    Drivers/ILI9341-TFT-LVGL/Display/ILI9341.c
    Drivers/ILI9341-TFT-LVGL/Display/LCDController.c
    Drivers/ILI9341-TFT-LVGL/Display/XPT2046.c
    Drivers/ILI9341-TFT-LVGL/Display/TouchController.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    Drivers/ILI9341-TFT-LVGL/Board
    Drivers/ILI9341-TFT-LVGL/Display
    Drivers                          # for lv_conf.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    FREERTOS                         # enables RTOS-aware code paths
    BOARD_STM32F446                  # or BOARD_STM32F411
)
```

---

## Repository Structure

```text
ILI9341-TFT-LVGL/                        ← add this folder to your Drivers/
├── Display/
│   ├── ILI9341.h / .c                   # Low-level SPI display driver
│   ├── LCDController.h / .c             # LVGL display port adapter
│   ├── XPT2046.h / .c                   # Low-level SPI touch driver
│   ├── TouchController.h / .c           # LVGL input device adapter
│   └── display_runtime_config.h         # Timing policy compile-time knobs
├── Board/
│   ├── board_config.h                   # Contract gate + board selector
│   ├── board_drivers.h / .c             # Immutable config getters
│   └── boards/
│       ├── board_stm32f446.h            # STM32F446 pin/SPI mappings
│       └── board_stm32f411.h            # STM32F411 template
├── datasheets/
│   ├── ILI9341.pdf
│   ├── xpt2046-datasheet.pdf
│   └── mb1136-default-c04_schematic.pdf
└── examples/
    └── LVGL_Lesson/01_Display/          # Working reference project (STM32F446)
        ├── Core/Src/main.c              # Init sequence + load_gui()
        ├── Core/Src/freertos.c          # Task definitions + LVGL tick timer
        └── Drivers/lv_conf.h            # LVGL configuration
```

---

## From-Scratch Setup on a New CubeMX Project

Follow every step in order. Steps marked **[CubeMX]** are done inside STM32CubeMX before
code generation. Steps marked **[Code]** are manual edits after generation.

---

### Step 1 — Clock Configuration [CubeMX]

1. Open the **Clock Configuration** tab.
2. Set SYSCLK to the maximum supported frequency for your MCU.
   - STM32F446RE: 180 MHz using HSE + PLL (PLLM=4, PLLN=180, PLLP=2).
   - Enable **Over-Drive** if running above 168 MHz.
3. Set APB1 ≤ 45 MHz, APB2 ≤ 90 MHz.
4. Verify SysTick source is HCLK (used by `HAL_Delay` and `HAL_GetTick`).

> If you skip over-drive and run at 180 MHz the chip will silently malfunction.

---

### Step 2 — SPI for the Display (ILI9341) [CubeMX]

The ILI9341 uses full-duplex SPI in Mode 0 (CPOL=0, CPHA=0).

1. Enable **SPI1** (or any SPI peripheral with a DMA-capable TX stream on your MCU).
2. Set:
   - Mode: **Full-Duplex Master**
   - Data Size: **8 Bits**
   - CPOL: **Low** / CPHA: **1 Edge**
   - NSS: **Software** (CS is driven manually as a GPIO)
   - Baud rate: start at **APB2 / 16** (~5.6 MHz) for bring-up; increase to /4 after
     confirming the display works.
   - First Bit: **MSB First**
3. Under **DMA Settings** for SPI1:
   - Add a DMA request for **SPI1_TX**.
   - Stream: **DMA2 Stream 3**, Channel: **3** (STM32F4xx mapping).
   - Direction: Memory → Peripheral.
   - Memory increment: **enabled**. Peripheral increment: **disabled**.
   - Data width (both): **Byte**.
   - Mode: **Normal**.
   - Priority: **Low** (raise to Medium if you see DMA starvation).
4. Under **NVIC Settings** for SPI1, enable **SPI1 global interrupt** at priority **5**
   (must be ≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`, which is 5 for FreeRTOS on
   STM32F4).
5. Enable **DMA2 global interrupt** at the same priority.

> SPI1 TX DMA is required for non-blocking display updates. Without it the LVGL flush
> callback blocks for every frame transfer.

---

### Step 3 — SPI for the Touch Controller (XPT2046) [CubeMX]

The XPT2046 uses the same SPI mode as ILI9341 but does not need DMA.

1. Enable **SPI2** (any spare SPI peripheral).
2. Set the same parameters as Step 2 except:
   - Baud rate: **APB1 / 8** (~5.6 MHz). Do not exceed the XPT2046 SPI clock limit of 2 MHz
     at 3.3 V; use APB1 / 16 (2.8 MHz) if you see noisy readings.
   - No DMA needed.
3. Enable **SPI2 global interrupt** at priority **5**.

---

### Step 4 — GPIO Pins [CubeMX]

Configure the following pins as **GPIO_Output**, push-pull, no pull, high-speed:

| Signal                    | Connected to          | User Label in CubeMX |
| ------------------------- | --------------------- | -------------------- |
| ILI9341 CS                | any free GPIO         | `CS`                 |
| ILI9341 DC (Data/Command) | any free GPIO         | `DC`                 |
| ILI9341 RESET             | any free GPIO         | `RESET`              |
| XPT2046 CS                | any free GPIO         | `T_CS`               |

Configure one pin as **GPIO_EXTI** (external interrupt) with falling-edge trigger:

| Signal               | Connected to          | User Label in CubeMX |
| -------------------- | --------------------- | -------------------- |
| XPT2046 IRQ (PENIRQ) | any EXTI-capable GPIO | `T_IRQ`              |

Under **NVIC**, enable the EXTI line for T_IRQ at priority **5**.

> Set T_IRQ pull to **No pull** if the display board has a hardware pull-up (most do).
> Set to **Pull-up** if unsure.

---

### Step 5 — FreeRTOS [CubeMX]

1. Enable **FreeRTOS** under **Middleware** → use **CMSIS-RTOS V2** API.
2. Set:
   - **Heap Management**: Heap_4 (supports `malloc`/`free` fragmentation).
   - **Total Heap Size**: ≥ 32768 bytes (32 KB). See memory notes below.
   - **USE_MUTEXES**: enabled.
   - **USE_RECURSIVE_MUTEXES**: enabled.
   - **USE_COUNTING_SEMAPHORES**: enabled.
3. Leave task creation in `freertos.c`; do **not** create the LVGL task in CubeMX — add
   it manually in the `USER CODE` blocks to keep it out of the regeneration zone.

#### FreeRTOSConfig.h key values [Code]

After generation, verify or set these in `Core/Inc/FreeRTOSConfig.h`:

```c
#define configENABLE_FPU                    1       // Required for Cortex-M4F
#define configTOTAL_HEAP_SIZE               (32 * 1024)
#define configMINIMAL_STACK_SIZE            128     // Words (512 bytes)
#define configTICK_RATE_HZ                  1000
#define configMAX_PRIORITIES                56
#define configUSE_MUTEXES                   1
#define configUSE_RECURSIVE_MUTEXES         1
#define configUSE_COUNTING_SEMAPHORES       1
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY  5
```

> `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY = 5` means all ISRs that call FreeRTOS
> API functions (like `xTaskNotifyFromISR`) must be at NVIC priority ≥ 5.
> The SPI and EXTI interrupts are set to exactly 5, which is the lowest valid priority.

---

### Step 6 — Optional: RTC and UART [CubeMX]

These are only needed if you want the time/date display and UART debug output from the
example. Skip if you are building a minimal display-only project.

**RTC:**

- Enable **RTC** with internal LSE or LSI clock.
- Clock source: LSE (32.768 kHz crystal) for accuracy; LSI for bring-up without a crystal.

**UART2 with DMA (for debug output):**

- Enable **USART2**, Asynchronous, 115200 baud.
- Add **DMA TX request** for USART2.

---

### Step 7 — Generate Code [CubeMX]

1. Set **Toolchain/IDE** to **CMake** (or **Makefile** / **STM32CubeIDE** as preferred).
2. Click **Generate Code**.

---

### Step 8 — Add LVGL [Code]

1. Clone or copy LVGL v9.2 into `Drivers/lvgl/`.
2. Copy `Drivers/lvgl/lv_conf_template.h` to `Drivers/lv_conf.h` (one level above `lvgl/`).
3. Set the required options in `lv_conf.h`:

```c
#define LV_USE_LVGL         1                      // Master enable (must be first)
#define LV_COLOR_DEPTH      16                     // RGB565 — matches ILI9341 pixel format
#define LV_MEM_SIZE         (16 * 1024U)           // 16 KB LVGL internal heap
#define LV_USE_OS           LV_OS_CMSIS_RTOS2      // Enable lv_lock()/lv_unlock()
```

1. In `CMakeLists.txt`, add LVGL sources and the `Drivers/` include path. LVGL provides a
   `CMakeLists.txt`; include it with `add_subdirectory(Drivers/lvgl)`.

---

### Step 9 — Add the Driver Package [Code]

Add this repository to your project (see [Installation](#installation) above), then add
the following to `CMakeLists.txt`:

```cmake
target_sources(${PROJECT_NAME} PRIVATE
    Drivers/ILI9341-TFT-LVGL/Board/board_drivers.c
    Drivers/ILI9341-TFT-LVGL/Display/ILI9341.c
    Drivers/ILI9341-TFT-LVGL/Display/LCDController.c
    Drivers/ILI9341-TFT-LVGL/Display/XPT2046.c
    Drivers/ILI9341-TFT-LVGL/Display/TouchController.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    Drivers/ILI9341-TFT-LVGL/Board
    Drivers/ILI9341-TFT-LVGL/Display
    Drivers                          # for lv_conf.h
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    FREERTOS                         # enables RTOS-aware code paths in the drivers
    BOARD_STM32F446                  # select your board (see Step 10)
)
```

---

### Step 10 — Create a Board Header [Code]

Duplicate `Board/boards/board_stm32f446.h` and edit it for your MCU. Every `BOARD_*`
macro maps to a CubeMX-generated symbol from `main.h` and `spi.h`.

```c
/* Display (ILI9341) on SPI1 */
#define BOARD_DISP_SPI_HANDLE        (&hspi1)
#define BOARD_DISP_CS_PORT           CS_GPIO_Port
#define BOARD_DISP_CS_PIN            CS_Pin
#define BOARD_DISP_DC_PORT           DC_GPIO_Port
#define BOARD_DISP_DC_PIN            DC_Pin
#define BOARD_DISP_RESET_PORT        RESET_GPIO_Port
#define BOARD_DISP_RESET_PIN         RESET_Pin
#define BOARD_DISP_HOR_RES           320u   // logical width after rotation
#define BOARD_DISP_VER_RES           240u   // logical height after rotation
#define BOARD_DISP_ROTATION          3u     // 0=portrait, 3=landscape

/* Touch (XPT2046) on SPI2 */
#define BOARD_TOUCH_SPI_HANDLE       (&hspi2)
#define BOARD_TOUCH_CS_PORT          T_CS_GPIO_Port
#define BOARD_TOUCH_CS_PIN           T_CS_Pin
#define BOARD_TOUCH_IRQ_PORT         T_IRQ_GPIO_Port
#define BOARD_TOUCH_IRQ_PIN          T_IRQ_Pin

/* Touch calibration defaults (raw ADC values at each screen edge) */
#define BOARD_TOUCH_RAW_X_MIN        262u
#define BOARD_TOUCH_RAW_X_MAX        1872u
#define BOARD_TOUCH_RAW_Y_MIN        230u
#define BOARD_TOUCH_RAW_Y_MAX        1872u
```

> Get calibration values by running `touch_calibrate()` once with `printf` enabled and
> noting the raw values printed for each corner. Then update these defaults.

Then register your new board in `Board/board_config.h`:

```c
#elif defined(BOARD_YOUR_MCU)
#include "boards/board_your_mcu.h"
```

And add the selector to your `CMakeLists.txt`:

```cmake
set(BOARD_TARGET "your_mcu" CACHE STRING "Board mapping target")

if(BOARD_TARGET STREQUAL "your_mcu")
    set(BOARD_TARGET_DEFINE BOARD_YOUR_MCU)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE ${BOARD_TARGET_DEFINE})
```

---

### Step 11 — Wire Initialization in main.c [Code]

Add inside `/* USER CODE BEGIN 2 */` (after all peripheral init, before `osKernelStart`):

```c
#include "lvgl.h"
#include "LCDController.h"
#include "TouchController.h"
#include "board_drivers.h"

lv_init();
lv_port_disp_init(Board_GetDisplayConfig());
TouchController_Init(Board_GetTouchConfig());
load_gui();   // your application UI
```

---

### Step 12 — FreeRTOS Tasks and LVGL Tick [Code]

Add the following inside `MX_FREERTOS_Init()` in `freertos.c`.

#### LVGL 1 ms tick timer

```c
osTimerId_t lvglTimerHandle;
const osTimerAttr_t lvglTimer_attributes = { .name = "lvglTimer" };

// inside MX_FREERTOS_Init():
lvglTimerHandle = osTimerNew(lvglTimerCallback, osTimerPeriodic, NULL, &lvglTimer_attributes);
osTimerStart(lvglTimerHandle, 1);
```

```c
void lvglTimerCallback(void *argument) {
    lv_tick_inc(1);
}
```

#### LVGL task (owns all LVGL API calls)

```c
osThreadId_t lvglTaskHandle;
const osThreadAttr_t lvglTask_attributes = {
    .name       = "lvglTask",
    .stack_size = 2048 * 4,           // 8 KB — increase if you see stack overflow
    .priority   = osPriorityLow,
};

void startLvglTask(void *argument) {
    for (;;) {
        osDelay(5);
        TouchController_Poll();
        lv_timer_handler();
    }
}
```

#### Calling LVGL from any other task

LVGL is not thread-safe. Any task that touches LVGL objects must hold the lock:

```c
lv_lock();
lv_label_set_text(my_label, "updated");
lv_unlock();
```

> `lv_lock()` / `lv_unlock()` are available when `LV_USE_OS != LV_OS_NONE`. With
> `LV_OS_CMSIS_RTOS2` they map to a CMSIS-RTOS2 mutex internally.

---

### Step 13 — Build and Verify [Code]

```bash
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug -D BOARD_TARGET=stm32f446
cmake --build build
```

Flash with your programmer and run the hardware smoke checklist:

1. Device boots without HardFault.
2. Display powers on; LVGL content renders with correct orientation.
3. Touch press is detected; drag tracks; corners map correctly.
4. Leave running 10+ minutes — no freeze or DMA deadlock.

---

## Board Selection

The `BOARD_TARGET` CMake variable selects which board header is compiled in:

```bash
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release -D BOARD_TARGET=stm32f411
```

Supported values:

| `BOARD_TARGET` | Compile define       | Board header                      |
| -------------- | -------------------- | --------------------------------- |
| `stm32f446`    | `BOARD_STM32F446`    | `Board/boards/board_stm32f446.h`  |
| `stm32f411`    | `BOARD_STM32F411`    | `Board/boards/board_stm32f411.h`  |

---

## Porting to a New Board

1. Copy `Board/boards/board_stm32f446.h` → `Board/boards/board_your_mcu.h`.
2. Update all `BOARD_*` macros to match your MCU's pin and SPI assignments.
3. Add a `#elif defined(BOARD_YOUR_MCU)` block in `Board/board_config.h`.
4. Add the new `BOARD_TARGET` value to your `CMakeLists.txt` mapping.
5. Build with `-D BOARD_TARGET=your_mcu`.

`board_config.h` will catch missing or invalid macros at compile time.

---

## Rotation Reference

| Value | Mode               | HOR_RES | VER_RES |
| ----- | ------------------ | ------- | ------- |
| 0     | Portrait (default) | 240     | 320     |
| 1     | Portrait flipped   | 240     | 320     |
| 2     | Landscape flipped  | 320     | 240     |
| 3     | Landscape          | 320     | 240     |

Set `BOARD_DISP_ROTATION` and update `BOARD_DISP_HOR_RES` / `BOARD_DISP_VER_RES` to
match. The display buffers in `LCDController.c` are sized from `BOARD_DISP_HOR_RES`.

---

## Memory Budget (STM32F446RE — 128 KB RAM)

| Component                               | Size       |
| --------------------------------------- | ---------- |
| FreeRTOS heap (`configTOTAL_HEAP_SIZE`) | 32 KB      |
| LVGL heap (`LV_MEM_SIZE`)               | 16 KB      |
| Display render buffers (2 × 10 rows)    | 12.8 KB    |
| LVGL task stack                         | 8 KB       |
| Main stack (MSP)                        | 4 KB       |
| All other task stacks                   | ~3 KB      |
| **Total**                               | **~76 KB** |

Reduce `LV_MEM_SIZE` or the buffer row count in `LCDController.c` for MCUs with less RAM.

---

## Deterministic Timing Policy

For real-time applications (e.g. motor control + UI on the same MCU), the display driver
can be configured to never block:

```bash
cmake ... -D DISPLAY_DETERMINISTIC_MODE=ON -D DISPLAY_DROP_FRAME_IF_DMA_BUSY=ON
```

When `DISPLAY_DROP_FRAME_IF_DMA_BUSY` is ON and a DMA transfer is still running when LVGL
tries to flush the next frame, the frame is dropped immediately. Worst-case flush latency
is bounded to the time for a single SPI transaction check.

All policy knobs are in `Display/display_runtime_config.h` and can be overridden
from CMake with `target_compile_definitions`:

| CMake option                      | Default | Effect                                      |
| --------------------------------- | ------- | ------------------------------------------- |
| `DISPLAY_DETERMINISTIC_MODE`      | ON      | Master switch for deterministic policy      |
| `DISPLAY_DROP_FRAME_IF_DMA_BUSY`  | ON      | Drop frame instead of waiting on DMA        |
| `DISPLAY_SPI_TIMEOUT_MS`          | 100     | Blocking SPI transaction timeout (ms)       |
| `DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS`| 20      | Max wait when drop-frame is disabled (ms)   |
| `DISPLAY_DMA_BUSY_WAIT_SLICE_MS`  | 1       | Polling granularity while waiting (ms)      |

---

## License

MIT

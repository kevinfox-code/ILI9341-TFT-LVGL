# Driver Rework Plan — ILI9341 + XPT2046, CubeMX-Decoupled, RTOS-Safe

**Audience:** an implementing code model. Follow the phases in order. Do not skip
acceptance criteria. Every file to create or modify is named explicitly. When a code
snippet is given, treat it as the required signature/contract — implementation details
inside function bodies may vary, but names, parameters, and semantics may not.

---

## 1. Goals

1. **Complete separation from CubeMX output.** No driver file may include `main.h`,
   `spi.h`, `gpio.h`, or any other CubeMX-generated header. The single connection point
   between the library and the CubeMX project is one app-provided header:
   `drv_constants.h`.
2. **Layered, hardened low level.** Layer 0 (OS abstraction + SPI/GPIO port) and
   Layer 1 (ILI9341, XPT2046 chip drivers) must work identically on bare metal and
   FreeRTOS (CMSIS-RTOS2), and must be usable with LVGL, TouchGFX, or no GUI at all.
3. **Thread safety.** All shared state guarded by CMSIS-RTOS2 mutexes; DMA completion
   signaled by a binary semaphore, not a volatile flag. Bare-metal builds compile the
   same call sites against a no-op/polling OS backend.
4. **Interrupts wired directly to the driver.** The driver owns ISR logic. The
   application (or a provided glue file) forwards HAL callbacks to named driver ISR
   entry points. No driver file defines `HAL_SPI_TxCpltCallback` or
   `HAL_GPIO_EXTI_Callback` unconditionally.
5. **LVGL is just a layer.** LVGL integration lives in an optional adapter directory
   that is not compiled unless requested. Same for a TouchGFX adapter.
6. **CMake-first.** The library is a proper CMake static-library target consumable
   with `add_subdirectory()`.
7. **Testing.** Host-based unit tests (run on the development machine, no hardware)
   plus an updated on-target smoke checklist.
8. **Updated example** reflecting the target project shape:
   `App/` (application + drivers) and `Profiles/board_1/` (all CubeMX output) at the
   project root.

## 2. Non-Goals

- No new display or touch controller support.
- No LVGL version upgrade (stay on v9.2.x).
- No change to the CubeMX `.ioc` peripheral configuration except what Phase 8 requires
  (regeneration into `Profiles/board_1/`).

---

## 3. Target Layout

### 3.1 This repository (the library — dropped into `App/drivers/` of user projects)

```
ILI9341-TFT-LVGL/
├── CMakeLists.txt                    # defines target: tft_drivers (+ options)
├── include/drv/                      # public headers, all include as "drv/xxx.h"
│   ├── drv_os.h                      # Layer 0: OS abstraction (OSAL)
│   ├── drv_spi.h                     # Layer 0: SPI bus port (bus object + lock)
│   ├── drv_gpio.h                    # Layer 0: GPIO pin port
│   ├── drv_config.h                  # contract gate: includes drv_constants.h, validates it
│   ├── drv_isr.h                     # driver ISR entry points (called from app ISRs/glue)
│   ├── drv_setup.h                   # one-call bring-up built from drv_constants.h macros
│   ├── ili9341.h                     # Layer 1: display chip driver
│   └── xpt2046.h                     # Layer 1: touch chip driver
├── src/
│   ├── os/
│   │   ├── drv_os_cmsis2.c           # CMSIS-RTOS2 backend
│   │   └── drv_os_baremetal.c        # bare-metal backend
│   ├── port/
│   │   ├── drv_spi_stm32.c           # STM32 HAL SPI implementation of drv_spi.h
│   │   └── drv_gpio_stm32.c          # STM32 HAL GPIO implementation of drv_gpio.h
│   ├── ili9341.c
│   ├── xpt2046.c
│   ├── drv_setup.c                   # ONLY file (with drv_isr_glue.c) that uses DRV_* macros
│   └── drv_isr_glue.c                # optional weak-callback glue (compile-time switchable)
├── adapters/
│   ├── lvgl/
│   │   ├── lv_ili9341.h / .c         # LVGL display port (replaces LCDController)
│   │   ├── lv_xpt2046.h / .c         # LVGL indev port (replaces TouchController)
│   │   └── lv_touch_calibrate.h / .c # on-screen calibration utility (moved out of driver)
│   └── touchgfx/
│       └── README.md                 # integration notes + skeleton (Phase 6)
├── templates/
│   └── drv_constants_template.h      # fully commented template the user copies to App/
├── tests/
│   ├── host/                         # host unit tests (Phase 7)
│   └── target/hardware_smoke_checklist.md
├── examples/LVGL_Lesson/01_Display/  # restructured example (Phase 8)
├── datasheets/
├── README.md
└── REWORK_PLAN.md                    # this file
```

The legacy `Display/` and `Board/` directories are **deleted** at the end of Phase 8
(keep them until then as reference).

### 3.2 A user project consuming the library (also the example's shape)

```
<project-root>/
├── CMakeLists.txt                    # top-level glue (user-owned, never regenerated)
├── App/
│   ├── CMakeLists.txt
│   ├── drv_constants.h               # THE connector: maps DRV_* macros → CubeMX symbols
│   ├── app_main.c / app_main.h       # app entry called from main.c USER CODE block
│   ├── gui/                          # LVGL screens (or TouchGFX Application)
│   └── drivers/                      # this repository (submodule or copy)
└── Profiles/
    └── board_1/                      # 100% CubeMX-generated output for board 1
        ├── board_1.ioc
        ├── Core/ (Inc, Src)          # main.c, stm32f4xx_it.c, FreeRTOSConfig.h, ...
        ├── Drivers/ (CMSIS, STM32F4xx_HAL_Driver)
        ├── Middlewares/ (FreeRTOS)
        ├── cmake/stm32cubemx/CMakeLists.txt
        ├── startup_stm32f446xx.s
        └── STM32F446XX_FLASH.ld
```

Rule: **nothing outside `Profiles/` is ever touched by CubeMX regeneration**, and the
only lines added inside `Profiles/` are within `USER CODE` blocks (documented in
Phase 8).

### 3.3 Layer diagram

```
┌──────────────────────────────────────────────────────────┐
│  Application (bare-metal loop / FreeRTOS tasks)           │
├─────────────────────────────┬────────────────────────────┤
│  adapters/lvgl (optional)   │  adapters/touchgfx (opt.)  │  Layer 2
├─────────────────────────────┴────────────────────────────┤
│  ili9341.c        xpt2046.c                               │  Layer 1 (chip drivers)
├───────────────────────────────────────────────────────────┤
│  drv_spi  drv_gpio  drv_os  drv_isr                       │  Layer 0 (ports + OSAL)
├───────────────────────────────────────────────────────────┤
│  STM32 HAL  ←  handles/pins supplied via drv_constants.h  │
└───────────────────────────────────────────────────────────┘
```

Include-direction rule (enforce by review in every phase): Layer 1 includes only
Layer 0 headers + libc. Layer 0 port implementations include the STM32 HAL family
header (via `drv_config.h`, see 4.3). **No layer includes `lvgl.h` except
`adapters/lvgl/`. No layer includes CubeMX headers except through `drv_constants.h`.**

---

## 4. Phase 1 — Layer 0: OSAL and ports

> **Status: DONE.** `include/drv/{drv_os,drv_gpio,drv_spi,drv_config,drv_isr,drv_setup}.h`,
> `src/os/{drv_os_cmsis2,drv_os_baremetal}.c`, `src/port/{drv_spi_stm32,drv_gpio_stm32}.c`,
> `src/drv_setup.c`, `templates/drv_constants_template.h`, and stub Layer-1 headers
> (`include/drv/ili9341.h`, `include/drv/xpt2046.h`) all created. Grep-based
> acceptance criteria verified passing. `ili9341.c`/`xpt2046.c` bodies land in Phase 2.

### 4.1 `include/drv/drv_os.h` — OS abstraction

```c
#ifndef DRV_OS_H_
#define DRV_OS_H_
#include <stdint.h>
#include <stdbool.h>

#define DRV_OS_WAIT_FOREVER 0xFFFFFFFFu

typedef void *drv_mutex_t;   /* NULL = invalid */
typedef void *drv_sem_t;     /* NULL = invalid */

typedef enum { DRV_OK = 0, DRV_ERR_TIMEOUT, DRV_ERR_PARAM, DRV_ERR_STATE, DRV_ERR_HW } drv_status_t;

drv_mutex_t  drv_mutex_create(const char *name);
drv_status_t drv_mutex_lock(drv_mutex_t m, uint32_t timeout_ms);
drv_status_t drv_mutex_unlock(drv_mutex_t m);

drv_sem_t    drv_sem_create_binary(const char *name);   /* created EMPTY (count 0) */
drv_status_t drv_sem_take(drv_sem_t s, uint32_t timeout_ms);
drv_status_t drv_sem_give(drv_sem_t s);                 /* usable from ISR and thread */

void     drv_delay_ms(uint32_t ms);
uint32_t drv_tick_ms(void);
bool     drv_in_isr(void);
bool     drv_os_kernel_running(void);   /* false before osKernelStart / on bare metal */
#endif
```

Backend selection: CMake option `DRV_USE_CMSIS_RTOS2` (default `ON`) compiles
`drv_os_cmsis2.c`, else `drv_os_baremetal.c`. Also define the compile definition
`DRV_USE_CMSIS_RTOS2=1` (or `0`) on the target so headers can branch if ever needed.
This replaces the old ad-hoc `FREERTOS` define everywhere — **remove all `#ifdef
FREERTOS` from driver code.**

**CMSIS-RTOS2 backend (`drv_os_cmsis2.c`)** — includes only `cmsis_os2.h`:
- `drv_mutex_create` → `osMutexNew` with `osMutexPrioInherit`. Not recursive.
- `drv_sem_create_binary` → `osSemaphoreNew(1, 0, ...)`.
- `drv_sem_give` → `osSemaphoreRelease` (CMSIS-RTOS2 is ISR-safe for release).
- `drv_sem_take` / `drv_mutex_lock` map `timeout_ms` → ticks with
  `osKernelGetTickFreq()` scaling; `DRV_OS_WAIT_FOREVER` → `osWaitForever`.
  If called while `drv_in_isr()` is true, must use timeout 0 semantics only;
  return `DRV_ERR_STATE` if a nonzero wait is requested from ISR.
- **Kernel-not-running guard:** before `osKernelGetState() == osKernelRunning`,
  `drv_mutex_lock/unlock` and `drv_sem_take` fall back to the bare-metal behavior
  below (init happens in `main()` before `osKernelStart()` — this guard is required,
  the current code deadlocks here if init calls `osDelay` pre-kernel).
  `drv_delay_ms` → `osDelay` when kernel running, else `HAL_Delay`.
- `drv_in_isr` → `(SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0` via `__get_IPSR()`.

**Bare-metal backend (`drv_os_baremetal.c`):**
- Mutexes: a `volatile uint8_t` lock flag guarded by PRIMASK critical section
  (`__disable_irq`/`__enable_irq` around test-and-set). Lock with timeout polls
  `drv_tick_ms()`. Single-context programs never contend; this still makes ISR/main
  interaction correct.
- Semaphores: `volatile uint8_t` flag; `take` polls flag with tick timeout;
  `give` sets it (works from ISR).
- `drv_delay_ms` → `HAL_Delay`; `drv_tick_ms` → `HAL_GetTick`;
  `drv_os_kernel_running` → `false`.

### 4.2 `include/drv/drv_spi.h` — SPI bus port

A *bus object* pairs a HAL handle with a lock and a DMA-done semaphore. Display and
touch may share one bus or use two — the setup layer decides (4.6).

```c
typedef struct drv_spi_bus drv_spi_bus_t;   /* opaque; defined in drv_spi_stm32.c */

drv_spi_bus_t *drv_spi_bus_create(void *hal_spi_handle /* SPI_HandleTypeDef* */);
drv_status_t drv_spi_bus_lock(drv_spi_bus_t *bus, uint32_t timeout_ms);
drv_status_t drv_spi_bus_unlock(drv_spi_bus_t *bus);

/* Blocking transfers. Caller must hold the bus lock. */
drv_status_t drv_spi_tx(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len, uint32_t timeout_ms);
drv_status_t drv_spi_txrx(drv_spi_bus_t *bus, const uint8_t *tx, uint8_t *rx, uint32_t len, uint32_t timeout_ms);

/* Non-blocking DMA TX. Caller must hold the bus lock and keep holding it until
 * completion is observed. len may exceed 65535: implementation chunks internally
 * is NOT required for the async call — instead it must return DRV_ERR_PARAM if
 * len > drv_spi_max_dma_len() and the caller chunks. */
drv_status_t drv_spi_tx_dma(drv_spi_bus_t *bus, const uint8_t *data, uint32_t len);
uint32_t     drv_spi_max_dma_len(void);          /* 65534 on STM32F4 HAL */
drv_status_t drv_spi_wait_tx_done(drv_spi_bus_t *bus, uint32_t timeout_ms); /* takes DMA sem */
bool         drv_spi_tx_busy(drv_spi_bus_t *bus);

/* ISR entry — forward from HAL TxCplt for this bus's handle (see drv_isr.h). */
void drv_spi_tx_complete_isr(drv_spi_bus_t *bus);
```

Implementation notes for `drv_spi_stm32.c`:
- Static pool of `DRV_SPI_MAX_BUSES` (default 2, CMake-overridable) bus structs; no
  heap. `drv_spi_bus_create` returns an existing bus if called twice with the same
  handle (this is how display+touch share a bus safely).
- `drv_spi_tx_complete_isr` must, **in this order**: wait for the SPI BSY flag to
  clear (bounded spin, ~10 µs max — HAL fires TxCplt from the DMA TC interrupt while
  the SPI shifter may still be emptying; raising CS before BSY clears truncates the
  last byte — this is a live bug in the current code), then invoke an optional
  per-bus `tx_done_hook` callback (used by ILI9341 to raise CS and notify LVGL),
  then `drv_sem_give` the bus's DMA semaphore.
- Provide `drv_spi_bus_set_tx_done_hook(bus, void (*hook)(void *ctx), void *ctx)`.
- All error paths from `HAL_SPI_Transmit_DMA` (busy/error return) must NOT leave the
  semaphore or any busy state inconsistent: on failure return `DRV_ERR_HW`
  immediately without touching the semaphore.
- The only ST include in this file: `drv_config.h` (which pulls the family HAL
  header — see 4.4).

### 4.3 `include/drv/drv_gpio.h` — GPIO port

```c
typedef struct { void *port; uint16_t pin; } drv_gpio_t;  /* port = GPIO_TypeDef* */
void drv_gpio_write(drv_gpio_t g, bool level);
bool drv_gpio_read(drv_gpio_t g);
```

Implemented in `drv_gpio_stm32.c` with `HAL_GPIO_WritePin/ReadPin`.

### 4.4 `include/drv/drv_config.h` — the contract gate

This replaces `Board/board_config.h`. Contents, in order:

1. `#include "drv_constants.h"` — resolved from the **application's** include path.
   The library never ships this file; `templates/drv_constants_template.h` is the
   copy source.
2. `#include DRV_HAL_HEADER` — `drv_constants.h` must define e.g.
   `#define DRV_HAL_HEADER "stm32f4xx_hal.h"`. This removes the hardcoded
   `stm32f4xx_hal.h` from `ili9341.h`/`xpt2046.h` (a current portability bug) —
   chip driver headers include `drv_config.h` only in their `.c` files; public
   headers use fixed-width types + `drv_gpio_t` and are HAL-free.
3. `#error` gates for every required macro (carry over the full list from the old
   `board_config.h`, renamed `BOARD_*` → `DRV_*`) plus the new required macros:
   `DRV_HAL_HEADER`, `DRV_DISP_SPI_MX_INIT()`, `DRV_TOUCH_SPI_MX_INIT()`,
   `DRV_TOUCH_IRQ_EXTI_LINE`.
4. The compile-time value validations (resolution non-zero, rotation 0–3,
   calibration min<max) — carry over unchanged, renamed.

### 4.5 `templates/drv_constants_template.h`

Full template with every macro documented. Required macros:

```c
/* HAL family */
#define DRV_HAL_HEADER               "stm32f4xx_hal.h"

/* CubeMX init hooks (allow the driver/app to re-init a peripheral for error
 * recovery or baud change without knowing MX names) */
#define DRV_DISP_SPI_MX_INIT()       MX_SPI1_Init()
#define DRV_TOUCH_SPI_MX_INIT()      MX_SPI2_Init()

/* Display (ILI9341) */
#define DRV_DISP_SPI_HANDLE          (&hspi1)
#define DRV_DISP_CS_PORT             CS_GPIO_Port
#define DRV_DISP_CS_PIN              CS_Pin
#define DRV_DISP_DC_PORT             DC_GPIO_Port
#define DRV_DISP_DC_PIN              DC_Pin
#define DRV_DISP_RESET_PORT          RESET_GPIO_Port
#define DRV_DISP_RESET_PIN           RESET_Pin
#define DRV_DISP_HOR_RES             320u
#define DRV_DISP_VER_RES             240u
#define DRV_DISP_ROTATION            3u

/* Touch (XPT2046) */
#define DRV_TOUCH_SPI_HANDLE         (&hspi2)   /* may equal DRV_DISP_SPI_HANDLE */
#define DRV_TOUCH_CS_PORT            T_CS_GPIO_Port
#define DRV_TOUCH_CS_PIN             T_CS_Pin
#define DRV_TOUCH_IRQ_PORT           T_IRQ_GPIO_Port
#define DRV_TOUCH_IRQ_PIN            T_IRQ_Pin
#define DRV_TOUCH_IRQ_EXTI_LINE      4u          /* EXTI line number for glue routing */

/* Touch calibration defaults */
#define DRV_TOUCH_RAW_X_MIN          262u
#define DRV_TOUCH_RAW_X_MAX          1872u
#define DRV_TOUCH_RAW_Y_MIN          230u
#define DRV_TOUCH_RAW_Y_MAX          1872u
```

The template must `#include "main.h"` and `#include "spi.h"` at its top — this is
the one and only place CubeMX headers enter the build, and it lives in the **app**,
not the library.

### 4.6 `include/drv/drv_setup.h` + `src/drv_setup.c`

The convenience bring-up that consumes the `DRV_*` macros so the app doesn't have to
build config structs by hand:

```c
drv_status_t DRV_Setup(void);                 /* creates buses, inits both chips */
ili9341_t   *DRV_GetDisplay(void);
xpt2046_t   *DRV_GetTouch(void);
```

`drv_setup.c` and `drv_isr_glue.c` are the **only** library sources allowed to
reference `DRV_*` macros. Everything else consumes config structs/handles — this is
what makes Layers 0–1 host-testable.

`DRV_Setup()` logic: create bus for `DRV_DISP_SPI_HANDLE`; create bus for
`DRV_TOUCH_SPI_HANDLE` (returns the same bus object if the handles are equal); fill
both chip config structs from macros; call `ILI9341_Init` and `XPT2046_Init`; return
first error encountered.

### Phase 1 acceptance criteria

- `tft_drivers` target compiles with both `-DDRV_USE_CMSIS_RTOS2=ON` and `OFF`.
- `grep -rn '"main.h"\|"spi.h"\|"gpio.h"' include/ src/ adapters/` returns nothing.
- `grep -rn 'lvgl.h' include/ src/` returns nothing (adapters excluded).
- `grep -rln 'DRV_DISP_\|DRV_TOUCH_' src/ | sort` prints exactly
  `src/drv_isr_glue.c` and `src/drv_setup.c`.

---

## 5. Phase 2 — Layer 1: harden the chip drivers

> **Status: DONE.** `src/ili9341.c` and `src/xpt2046.c` implemented against the
> Phase-1 headers. Preserves the exact known-good init byte sequence and MADCTL
> values; adds bus locking/unlock on every path, DMA chunking for full-frame
> flushes, `DrawPixel`, capped `FillScreen` line buffer, calibration-mutex-guarded
> touch calibration, and rotation-aware `ReadPoint` (rotation 3 matches the
> legacy empirically-validated mapping; 0/1/2 derived from it and **must be
> re-verified on hardware** — flagged for the Phase 7 smoke checklist).
> `grep -n 'HAL_' include/drv/ili9341.h include/drv/xpt2046.h` → empty.

### 5.1 `include/drv/ili9341.h` / `src/ili9341.c`

Rewrite from the current `Display/ILI9341.c`. Public API:

```c
typedef struct {
    drv_spi_bus_t *bus;
    drv_gpio_t cs, dc, reset;
    uint16_t hor_res, ver_res;   /* logical, after rotation */
    uint8_t  rotation;           /* 0..3 */
} ili9341_config_t;

typedef struct ili9341 ili9341_t;   /* opaque instance; static storage inside driver */

/* done_cb fires in ISR context when an async flush completes (after CS raised). */
typedef void (*ili9341_done_cb_t)(void *user);

drv_status_t ILI9341_Init(ili9341_t **out, const ili9341_config_t *cfg);
drv_status_t ILI9341_SetRotation(ili9341_t *d, uint8_t rot);
drv_status_t ILI9341_FillScreen(ili9341_t *d, uint16_t rgb565);
drv_status_t ILI9341_DrawPixel(ili9341_t *d, uint16_t x, uint16_t y, uint16_t rgb565);
drv_status_t ILI9341_Flush(ili9341_t *d, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                           const uint8_t *px, uint32_t len_bytes, uint32_t timeout_ms); /* blocking */
drv_status_t ILI9341_FlushAsync(ili9341_t *d, uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                                const uint8_t *px, uint32_t len_bytes,
                                ili9341_done_cb_t cb, void *user);
bool         ILI9341_FlushBusy(ili9341_t *d);
drv_status_t ILI9341_WaitFlushDone(ili9341_t *d, uint32_t timeout_ms);
drv_status_t ILI9341_Sleep(ili9341_t *d, bool on);   /* 0x10 / 0x11 + delays */
```

Required behavior (each bullet fixes a current bug or hardening gap):

1. **Bus locking:** every public function acquires `drv_spi_bus_lock` for the whole
   transaction (command + data + CS bracket) and releases it before returning —
   except `FlushAsync`, which acquires the lock and releases it **in the completion
   path** (the tx-done hook), so a concurrent XPT2046 read on a shared bus blocks
   until the frame finishes rather than corrupting the transfer.
2. **Async completion:** `FlushAsync` uses `drv_spi_tx_dma`. The bus tx-done hook
   (registered at init, context = the `ili9341_t*`) raises CS, releases the bus lock,
   then calls the user `done_cb`. `WaitFlushDone` wraps `drv_spi_wait_tx_done`.
   The `volatile bool ili_dma_busy` global and the `dma_display` LVGL pointer are
   **deleted** — the driver has no LVGL knowledge.
3. **Chunking:** if `len_bytes > drv_spi_max_dma_len()`, `FlushAsync` transfers in
   chunks; intermediate chunks re-arm DMA from the tx-done hook (ISR context), only
   the final chunk performs the CS-raise/unlock/callback sequence. This fixes the
   current `w * h * 2` uint16 overflow (anything ≥ 128 rows at 320 px wide today
   silently truncates).
4. **Failure rollback:** if `drv_spi_tx_dma` fails mid-setup, raise CS, unlock the
   bus, and return `DRV_ERR_HW`. Never leave CS low or the lock held on any error
   path (audit every early return).
5. **Init concurrency guard:** `ILI9341_Init` runs the existing init command table
   (keep the byte sequence exactly as today — it is known-good on hardware) under
   the bus lock, and is callable before the kernel starts (the OSAL guard from 4.1
   makes the delays fall back to `HAL_Delay`).
6. **DrawPixel implemented** (declared today but missing — a link error waiting to
   happen): SetWindow(x,y,x,y) + 2 data bytes.
7. **FillScreen** sizes its line buffer from `cfg->hor_res` with a compile-time cap
   `ILI9341_MAX_LINE_PX` (default 320, CMake-overridable) and returns
   `DRV_ERR_PARAM` if `hor_res` exceeds the cap — no silent stack smash.
8. **No `LV_ATTRIBUTE_MEM_ALIGN`, no `lv_display_t`, no `HAL_SPI_TxCpltCallback`**
   anywhere in this file.

### 5.2 `include/drv/xpt2046.h` / `src/xpt2046.c`

```c
typedef struct {
    drv_spi_bus_t *bus;
    drv_gpio_t cs;
    drv_gpio_t irq;              /* irq.port == NULL → no PENIRQ wired */
    uint16_t raw_x_min, raw_x_max, raw_y_min, raw_y_max;
} xpt2046_config_t;

typedef struct xpt2046 xpt2046_t;

drv_status_t XPT2046_Init(xpt2046_t **out, const xpt2046_config_t *cfg);
bool         XPT2046_PenDown(xpt2046_t *t);                 /* reads PENIRQ GPIO */
bool         XPT2046_ReadRaw(xpt2046_t *t, uint16_t *rx, uint16_t *ry);
bool         XPT2046_ReadPoint(xpt2046_t *t, uint16_t *px, uint16_t *py,
                               uint16_t hor_res, uint16_t ver_res, uint8_t rotation);
void         XPT2046_SetCalibration(xpt2046_t *t, uint16_t xmin, uint16_t xmax,
                                    uint16_t ymin, uint16_t ymax);   /* validates min<max */
void         XPT2046_GetCalibration(xpt2046_t *t, uint16_t *xmin, uint16_t *xmax,
                                    uint16_t *ymin, uint16_t *ymax);
void         XPT2046_PenIRQ_ISR(xpt2046_t *t);   /* forward EXTI here; see drv_isr.h */
typedef void (*xpt2046_pen_cb_t)(void *user);
void         XPT2046_SetPenDownCallback(xpt2046_t *t, xpt2046_pen_cb_t cb, void *user);
```

Required behavior:

1. `ReadRaw` takes the bus lock with a short timeout (`DRV_TOUCH_BUS_TIMEOUT_MS`,
   default 10 ms, compile-overridable); on timeout return `false` (treat as no
   touch). Keep the current sampling scheme (discard first conversion per axis,
   average two). Keep the last command as a power-down variant (`0xD0`→ send final
   read with PD bits 00, i.e. append one `0x90 & ~0x03`-style transaction or simply
   send command `0x00` end byte) so PENIRQ is re-enabled after the burst — verify
   against the datasheet in `datasheets/xpt2046-datasheet.pdf`; today the code
   leaves PD bits at 01/11 patterns implicitly, which can mask PENIRQ.
2. `ReadPoint` performs the raw→pixel mapping **inside the driver** using the
   calibration and the rotation parameter — the axis swap currently hidden in
   `TouchController.c` (`map_x` consumes raw *Y*) moves here, table-driven by
   rotation so all four rotations work, with clamping.
3. Calibration values live in the instance, guarded by a mutex created at init
   (they're written by a calibrate routine while the LVGL task reads them). This
   replaces `Board/board_drivers.c` calibration state entirely.
4. `XPT2046_PenIRQ_ISR` records pen-down (for the LVGL adapter to consume) and
   invokes the registered callback (ISR context — the adapter uses it to wake the
   GUI task). **No SPI access in the ISR.**
5. Header is HAL-free: no `stm32f4xx_hal.h` include (uses `drv_gpio_t`).

### Phase 2 acceptance criteria

- Library compiles for the target (arm-none-eabi) in both OS modes.
- `grep -n 'HAL_' include/drv/ili9341.h include/drv/xpt2046.h` → nothing.
- Every early-return path in `ili9341.c` verified (by reading, and by the Phase 7
  mock tests) to leave CS high and the bus unlocked.
- Old `Display/ILI9341.c`, `XPT2046.c` behaviors preserved: identical init byte
  sequence, identical MADCTL values per rotation (0x48/0x28/0x88/0xE8), identical
  touch sampling order.

---

## 6. Phase 3 — Interrupt wiring

> **Status: DONE.** `drv_setup.c` implements `DRV_ISR_DisplaySpiTxCplt`/
> `DRV_ISR_TouchPenDown` and, when `USE_HAL_SPI_REGISTER_CALLBACKS==1`, registers
> its own `HAL_SPI_TX_COMPLETE_CB_ID` callback in `DRV_Setup()`. `src/drv_isr_glue.c`
> provides the fallback global `HAL_SPI_TxCpltCallback`/`HAL_GPIO_EXTI_Callback`
> definitions, compiled out entirely when register-callbacks mode is active (the
> CMake `DRV_PROVIDE_HAL_CALLBACKS` gate is added in Phase 6; the `nm` symbol-absence
> check will be run once that target exists).
> `grep -rln 'DRV_DISP_\|DRV_TOUCH_' src/` → `drv_isr_glue.c`, `drv_setup.c`, plus
> `xpt2046.c` (only for the header-defined default `DRV_TOUCH_BUS_TIMEOUT_MS` policy
> constant named per §5.2, not a board-macro leak).

### 6.1 `include/drv/drv_isr.h`

Single header declaring every ISR entry point an application must route:

```c
void DRV_ISR_DisplaySpiTxCplt(void);   /* from HAL_SPI_TxCpltCallback for display SPI  */
void DRV_ISR_TouchPenDown(void);       /* from EXTI callback for DRV_TOUCH_IRQ_PIN     */
```

Implemented in `drv_setup.c` (they resolve the singleton instances and forward to
`drv_spi_tx_complete_isr` / `XPT2046_PenIRQ_ISR`). They must be safe no-ops if
`DRV_Setup` has not run.

### 6.2 `src/drv_isr_glue.c` — default wiring

Compiled only when CMake option `DRV_PROVIDE_HAL_CALLBACKS=ON` (default ON). Two
strategies inside, chosen by the HAL's own switch:

- `#if USE_HAL_SPI_REGISTER_CALLBACKS == 1` → `DRV_Setup` registers
  `HAL_SPI_RegisterCallback(..., HAL_SPI_TX_COMPLETE_CB_ID, ...)` at init and the
  glue defines nothing global. (Preferred; enable "Register Callbacks" for SPI in
  CubeMX → Project Manager → Advanced Settings.)
- `#else` → glue defines `HAL_SPI_TxCpltCallback` and `HAL_GPIO_EXTI_Callback`,
  filtering by `DRV_DISP_SPI_HANDLE` / `DRV_TOUCH_IRQ_PIN` and forwarding to the
  `DRV_ISR_*` functions. Both must fall through silently for other
  handles/pins so the app can extend them — but note in the header comment that an
  app defining its own `HAL_GPIO_EXTI_Callback` must set
  `DRV_PROVIDE_HAL_CALLBACKS=OFF` and forward manually (link-time duplicate
  otherwise).

The `HAL_GPIO_EXTI_Callback` in `TouchController.c` today notifies
`lvglTaskHandle` directly with `xTaskNotifyFromISR` — that coupling is deleted;
waking the GUI task is now the LVGL adapter's job via
`XPT2046_SetPenDownCallback`.

### 6.3 NVIC contract (documentation, README section)

Document that all three IRQs (display SPI or its DMA stream, touch EXTI) must be at
NVIC priority ≥ `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY` (5) — carry the
existing README text over.

### Phase 3 acceptance criteria

- With `DRV_PROVIDE_HAL_CALLBACKS=OFF`, `nm` on the library archive shows no
  `HAL_SPI_TxCpltCallback` / `HAL_GPIO_EXTI_Callback` symbols.
- Example project (Phase 8) touches `stm32f4xx_it.c` **zero times** outside what
  CubeMX generated.

---

## 7. Phase 4 — Layer 2: LVGL adapter (`adapters/lvgl/`)

> **Status: DONE.** `adapters/lvgl/lv_ili9341.{h,c}`, `lv_xpt2046.{h,c}`,
> `lv_touch_calibrate.{h,c}` implemented, plus `include/drv/drv_policy.h` for the
> `DRV_DROP_FRAME_IF_BUSY` / `DRV_DMA_BUSY_WAIT_TIMEOUT_MS` knobs. Added a small
> non-breaking addition to the Layer-1 contract: `ILI9341_GetResolution()` (not
> in the original snippet, needed so the adapter can size `lv_display_create()`
> without the adapter reaching into the opaque `ili9341_t`). The stalled-transfer
> double-flush-ready race is closed via a per-adapter `pending` pointer cleared
> under a critical section by whichever of {done_cb, timeout path} gets there
> first. `grep -rl 'lvgl.h' include/ src/ adapters/` → only the three
> `adapters/lvgl/*.h` files. `DRV_ADAPTER_LVGL=OFF`/`nm` zero-`lv_`-symbols check
> deferred to Phase 6 once the CMake target exists.

Compiled only when CMake option `DRV_ADAPTER_LVGL=ON`. Links against the app's
`lvgl` target. Replaces `LCDController.*` and `TouchController.*`.

### 7.1 `lv_ili9341.h/.c`

```c
lv_display_t *lv_ili9341_create(ili9341_t *disp, uint8_t *buf1, uint8_t *buf2, uint32_t buf_bytes);
```

- Caller owns the buffers (sized `hor_res * N_rows * 2`, `LV_ATTRIBUTE_MEM_ALIGN`).
  The adapter no longer hides `static` buffers sized by board macros — the example
  shows the recommended 10-row ping-pong allocation.
- Flush callback policy (keep the `display_runtime_config.h` knobs, renamed with
  `DRV_` prefix and moved to `include/drv/drv_policy.h`):
  - `DRV_DROP_FRAME_IF_BUSY=1`: if `ILI9341_FlushBusy`, call
    `lv_display_flush_ready` and return.
  - else: `ILI9341_WaitFlushDone(d, DRV_DMA_BUSY_WAIT_TIMEOUT_MS)`; on timeout,
    flush_ready and return (frame dropped, no cancel-callback dance — the done_cb
    for the *stalled* transfer must be prevented from double-firing: pass the LVGL
    display through the `done_cb` `user` pointer per flush and clear it under a
    critical section, mirroring today's `ILI9341_CancelFlushCallback` intent but
    inside the adapter).
  - `done_cb` (ISR context) calls `lv_display_flush_ready`.
- Uses `LV_DISPLAY_RENDER_MODE_PARTIAL`.

### 7.2 `lv_xpt2046.h/.c`

```c
lv_indev_t *lv_xpt2046_create(xpt2046_t *touch, lv_display_t *disp);
```

- The indev `read_cb` calls `XPT2046_ReadPoint` directly (LVGL calls read_cb from
  `lv_timer_handler` in the GUI task — polling in task context, SPI is
  mutex-guarded, so the separate `TouchController_Poll` + volatile-globals pattern
  is deleted).
- Registers a pen-down callback that does `lv_indev_read(...)` scheduling or simply
  wakes the GUI task if the app registered a waker: provide
  `lv_xpt2046_set_wake_cb(void (*cb)(void*), void *user)`.

### 7.3 `lv_touch_calibrate.h/.c`

Move `touch_calibrate()` here, renamed `lv_xpt2046_calibrate(xpt2046_t*, lv_display_t*)`.
Remove `printf` calls — report through a caller-supplied
`void (*log_cb)(const char *fmt, ...)` (NULL = silent). Results are applied with
`XPT2046_SetCalibration` and also returned via an out-struct so the app can persist
them.

### Phase 4 acceptance criteria

- `DRV_ADAPTER_LVGL=OFF` build contains zero references to `lv_` symbols
  (`nm` check).
- Example runs: render + touch works as before on STM32F446 hardware
  (hardware smoke checklist).

---

## 8. Phase 5 — TouchGFX adapter notes (`adapters/touchgfx/README.md`)

Documentation + skeleton only (TouchGFX projects are generated by TouchGFX
Designer; the adapter can't be a compiled library without a TouchGFX tree). Write:

1. How `ILI9341_FlushAsync` maps to TouchGFX's `LCD`/`HAL::flushFrameBuffer` — a
   skeleton `TouchGFXGeneratedHAL`-derived class snippet calling
   `ILI9341_Flush/FlushAsync` and `ILI9341_WaitFlushDone` from
   `HAL::endFrame`.
2. How `XPT2046_ReadPoint` implements
   `touchgfx::TouchController::sampleTouch(int32_t& x, int32_t& y)` — skeleton class
   included.
3. Note that `drv_constants.h` + `DRV_Setup()` are identical in TouchGFX projects;
   only Layer 2 differs.

---

## 9. Phase 6 — CMake

### 9.1 Library `CMakeLists.txt` (repo root)

```cmake
cmake_minimum_required(VERSION 3.22)
project(tft_drivers C)

option(DRV_USE_CMSIS_RTOS2      "Use CMSIS-RTOS2 OSAL backend"        ON)
option(DRV_PROVIDE_HAL_CALLBACKS "Compile default HAL callback glue"  ON)
option(DRV_ADAPTER_LVGL         "Compile LVGL adapter"                OFF)
option(DRV_DROP_FRAME_IF_BUSY   "Drop LVGL frame if flush busy"       ON)
# + DRV_SPI_TIMEOUT_MS, DRV_DMA_BUSY_WAIT_TIMEOUT_MS cache strings (carry over)

add_library(tft_drivers STATIC ...)
target_include_directories(tft_drivers PUBLIC include)
```

Rules:
- The library does **not** know HAL include paths or `drv_constants.h`'s location.
  The consumer must do:
  `target_link_libraries(tft_drivers PUBLIC stm32cubemx)` — wrong direction; instead
  the consumer provides an interface target:
  `target_link_libraries(tft_drivers PRIVATE drv_platform)` where the **app**
  defines `drv_platform` as an INTERFACE library carrying: CubeMX include dirs, the
  directory containing `drv_constants.h`, `cmsis_os2` includes, and MCU defines.
  Document this contract at the top of the CMakeLists. If `DRV_ADAPTER_LVGL`, also
  require the consumer's `lvgl` target to exist.
- Board selection defines (`BOARD_STM32F446` etc.) are **deleted** — board variation
  is now entirely `drv_constants.h` content chosen by which `Profiles/<board>` the
  top-level CMake selects. The `Board/boards/` header mechanism is retired.

### 9.2 Guard rails

Add `cmake/check_layering.cmake` executed as a build-time custom target
`layering_check` that greps (CMake `file(STRINGS)` or a small script) for the
forbidden includes listed in the Phase 1 acceptance criteria and FATAL_ERRORs on
violation. Wire it into CTest.

---

## 10. Phase 7 — Testing

### 10.1 Host unit tests (`tests/host/`)

Standalone CMake project (`tests/host/CMakeLists.txt`) built for the **host**
(macOS/Linux), not ARM. Vendor Unity (single `unity.c/h`, MIT) into
`tests/host/vendor/unity/`.

Because Layer 1 consumes only `drv_spi.h`/`drv_gpio.h`/`drv_os.h`, provide mocks in
`tests/host/mocks/`:

- `mock_spi.c` — records every call (`tx`, `txrx`, `tx_dma`, lock/unlock) into an
  inspectable log with byte payloads; scriptable return values; fake DMA completion
  via manual `mock_spi_fire_tx_complete()`.
- `mock_gpio.c` — records pin writes with ordering relative to SPI calls (needed to
  assert CS/DC bracketing).
- `mock_os.c` — deterministic fake: mutexes count lock/unlock and detect imbalance
  at test teardown; semaphores are counters; `drv_tick_ms` is manually advanced.

Required test files and the cases each must contain:

`test_ili9341_init.c`
- Init sends the reset pulse (GPIO ordering) then the exact known-good command/data
  byte sequence (assert full byte log).
- Init acquires and releases the bus lock (mock_os balance check).

`test_ili9341_window.c`
- SetWindow/Flush emit correct 0x2A/0x2B/0x2C encoding for boundary coords
  (0,0)-(319,239) and a 1×1 area.
- MADCTL per rotation = 0x48/0x28/0x88/0xE8.

`test_ili9341_flush_async.c`
- FlushAsync: DC high + CS low before `tx_dma`; after
  `mock_spi_fire_tx_complete()`: CS raised, bus unlocked, done_cb called exactly
  once.
- Chunking: len = 153600 (full 320×240 frame) produces ⌈153600/65534⌉ = 3 DMA calls
  with correct offsets/lengths; CS raised only after the last.
- `tx_dma` failure: returns DRV_ERR_HW, CS high, lock released, done_cb NOT called.
- Busy: second FlushAsync while busy returns DRV_ERR_STATE (or blocks per policy —
  match implementation) without corrupting state.

`test_xpt2046_read.c`
- ReadRaw command sequence: X-discard, X, X, Y-discard, Y, Y averaging math.
- Bus lock timeout → returns false, no CS glitch (CS never went low, or went low
  and back high — assert bracket integrity).
- Pen-up (`irq` GPIO high) → ReadRaw returns false without SPI traffic.

`test_xpt2046_mapping.c`
- ReadPoint mapping for all 4 rotations: corner raw values → expected pixel corners;
  clamping below min / above max; degenerate calibration rejected by
  SetCalibration.

`test_osal_baremetal.c`
- Bare-metal semaphore take timeout advances with mocked tick; give-from-"ISR"
  releases a pending take.

Wire all of it to CTest (`add_test` per binary). Add
`tests/host/run.sh` that configures + builds + runs ctest.

### 10.2 On-target tests

- Update `tests/target/hardware_smoke_checklist.md` (move from
  `examples/.../tests/full_system/`): boot, render, rotation check, touch corners,
  10-minute soak, and two new items: (a) concurrent stress — a second task hammers
  `XPT2046_ReadRaw` while LVGL animates, on shared-bus config, for 5 minutes, no
  corruption/deadlock; (b) DMA soak with `DRV_DROP_FRAME_IF_BUSY=0`.
- Add `tests/target/selftest.c` (optional compile into example via
  `-DDRV_BUILD_SELFTEST=ON`): fills screen R/G/B, draws a gradient via Flush,
  reports pass/fail over whatever the app's log hook is.

### Phase 7 acceptance criteria

- `cd tests/host && ./run.sh` → all tests pass on the host with no ARM toolchain.
- CTest includes `layering_check`.

---

## 11. Phase 8 — Restructure the example project

Target: `examples/LVGL_Lesson/01_Display/` becomes the reference for the
`App/` + `Profiles/board_1/` shape (§3.2).

Steps:

1. Create `Profiles/board_1/` and move ALL CubeMX output into it: `Core/`,
   `Drivers/CMSIS/`, `Drivers/STM32F4xx_HAL_Driver/`, `Middlewares/`,
   `cmake/stm32cubemx/`, `cmake/gcc-arm-none-eabi.cmake`, `startup_stm32f446xx.s`,
   `STM32F446XX_FLASH.ld`, `01_Display.ioc` (rename `board_1.ioc`),
   `newlib_lock_glue.c`, `stm32_lock.h`. Update the `.ioc`'s project path settings
   comment in README (regeneration must target this folder).
2. Create `App/` with:
   - `drv_constants.h` (from the template, F446 values — same content as the old
     `board_stm32f446.h` renamed to `DRV_*` plus the new macros).
   - `app_main.h/c` exposing `void App_Init(void)` (pre-kernel: `lv_init`,
     `DRV_Setup`, `lv_ili9341_create` with app-owned buffers, `lv_xpt2046_create`,
     `load_gui`) and `void App_CreateTasks(void)` (GUI task + 1 ms tick timer —
     move the bodies out of `freertos.c` here).
   - `gui/` — move `load_gui` and demo screens out of `main.c`.
   - `App/CMakeLists.txt` — defines `drv_platform` INTERFACE lib (include dirs:
     `App/`, CubeMX Inc dirs from `Profiles/board_1`, FreeRTOS/CMSIS-RTOS2 include
     dirs; defines: `STM32F446xx`, `USE_HAL_DRIVER`), does
     `add_subdirectory(<repo-root> tft_drivers)` with `DRV_ADAPTER_LVGL=ON`,
     `add_subdirectory` LVGL (LVGL source stays where the example keeps it —
     move to `App/lvgl/` or keep `Drivers/lvgl` under App; put `lv_conf.h` in
     `App/`).
   - LVGL note: keep `LV_USE_OS LV_OS_CMSIS_RTOS2` in `lv_conf.h`.
3. Top-level `CMakeLists.txt`: keeps toolchain/preset handling, includes
   `Profiles/board_1/cmake/stm32cubemx` for the `stm32cubemx` target, adds `App/`,
   links `${PROJECT_NAME}` ← `stm32cubemx`, `lvgl`, `tft_drivers`. Add cache var
   `PROFILE` (default `board_1`) selecting `Profiles/${PROFILE}` so adding
   `board_2` later is a one-folder + one-cache-var affair.
4. `Core/Src/main.c` USER CODE blocks shrink to: `#include "app_main.h"`,
   `App_Init();` before `osKernelStart` region. `freertos.c` USER CODE calls
   `App_CreateTasks();`. Remove the old direct includes of `ILI9341.h`,
   `LCDController.h`, `TouchController.h`, the `load_gui` body, and the old
   lvglTask/lvglTimer bodies from CubeMX files (they move to `App/`).
   `stm32f4xx_it.c` needs **no user edits** (glue handles callbacks).
5. Delete `Display/`, `Board/` at repo root and the example's old driver source
   listing in CMake; delete `plan-progress.md`; delete root `lvgl-release-v9.2.zip`
   if no longer referenced (extract into the example's LVGL location first if it is
   the LVGL source of record).
6. Build both configs:
   `cmake --preset Debug && cmake --build --preset Debug` (or the equivalent
   explicit commands) with RTOS ON; then a bare-metal compile check of the library
   alone with `DRV_USE_CMSIS_RTOS2=OFF` (library target only — the example app
   itself stays FreeRTOS).

### Phase 8 acceptance criteria

- Example compiles clean (no warnings from `tft_drivers` sources at `-Wall -Wextra`).
- `git diff` inside `Profiles/board_1/Core` after a CubeMX regeneration shows
  changes only in generated regions (manually verify USER CODE blocks contain only
  the two `App_*` calls and one include).
- Hardware smoke checklist passes on STM32F446 (user runs this; flag it as the
  final gate).

---

## 12. Phase 9 — Documentation

1. Rewrite `README.md`: new layout, the `drv_constants.h` contract (full macro
   table), the `drv_platform` CMake contract, bring-up recipe for the
   `App/` + `Profiles/` shape, interrupt wiring section (both strategies from
   Phase 3), bare-metal usage section (no-RTOS main loop example), TouchGFX
   pointer, testing instructions. Keep the CubeMX peripheral setup steps 1–7
   (they're accurate) but retarget step 7 to generate into `Profiles/board_1/` and
   recommend enabling SPI "Register Callbacks".
2. Update `RELEASE_NOTES.md` with a breaking-changes section (old API → new API
   table: `lv_port_disp_init` → `lv_ili9341_create`, `TouchController_Init` →
   `lv_xpt2046_create`, `Board_Get*` → deleted, `BOARD_*` → `DRV_*`, etc.).

---

## Appendix A — Known bugs in the current code (must be fixed by the phases above)

| # | Location | Bug | Fixed in |
|---|----------|-----|----------|
| 1 | `Display/ILI9341.c:230` | Defines global `HAL_SPI_TxCpltCallback` — collides with any other SPI user in the app; fires for wrong handle filtered only by pointer compare | Phase 3 |
| 2 | `Display/ILI9341.c:237` | CS raised in TxCplt while SPI BSY may still be shifting the last byte(s) → truncated final pixels | Phase 1 (4.2) |
| 3 | `Display/ILI9341.c:222` | `HAL_SPI_Transmit_DMA(..., w*h*2)` — `uint16_t` size param overflows for areas ≥ 32768 px (e.g. full-frame flush) | Phase 2 (5.1.3) |
| 4 | `Display/ILI9341.c:216` | `ILI9341_DrawBitmapDMA` ignores `HAL_SPI_Transmit_DMA` return; on HAL_BUSY the CS stays low, `ili_dma_busy` stays true forever → permanent frame drop / deadlock | Phase 2 (5.1.4) |
| 5 | `Display/ILI9341.c:15` + `LCDController.c` | `volatile bool ili_dma_busy` read-modify-write race between flush (task) and ISR; no memory barrier, no semaphore | Phase 1/2 |
| 6 | `Display/ILI9341.c:225` + `LCDController.c:79` | `ILI9341_CancelFlushCallback` races the ISR: ISR can be between the `dma_display` NULL-check and `lv_display_flush_ready` when the task cancels → double flush_ready | Phase 4 (7.1) |
| 7 | `Display/ILI9341.h:27` | `ILI9341_DrawPixel` declared, never defined → link error if used | Phase 2 (5.1.6) |
| 8 | `Display/ILI9341.c:2-3` | Low-level driver includes `main.h` and `lvgl.h` — CubeMX and GUI coupling | Phases 1–2 |
| 9 | `Display/XPT2046.h:8`, `ILI9341.h:4` | Hardcoded `stm32f4xx_hal.h` — F4-only | Phase 1 (4.4) |
| 10 | `Display/TouchController.c:192` | Defines global `HAL_GPIO_EXTI_Callback` — collides with app EXTI users; hard-references `lvglTaskHandle` extern | Phase 3 |
| 11 | `Display/TouchController.c:26-28` | `touch_x/touch_y/touch_pressed` volatile globals written by poll, read by LVGL — torn reads possible (16-bit pairs, no lock) | Phase 4 (7.2 deletes the pattern) |
| 12 | `Display/TouchController.c:37-50` | Axis mapping hardcodes one rotation (raw-Y→X, raw-X→Y); wrong for rotations 0/1/2 | Phase 2 (5.2.2) |
| 13 | `Display/XPT2046.c` | No bus lock — concurrent display+touch on a shared SPI bus corrupts transfers; PD bits never restored to re-enable PENIRQ | Phase 2 (5.2.1) |
| 14 | `Display/ILI9341.c:154` | `line_buf[320*2]` fixed-size stack buffer; boards with `hor_res > 320` overflow the stack | Phase 2 (5.1.7) |
| 15 | `Display/TouchController.c:74` | `touch_calibrate` mixes LVGL calls, `printf`, blocking loops into the "driver" layer | Phase 4 (7.3) |
| 16 | OSAL-wide | `osDelay`/mutex calls made before `osKernelStart` hang; no kernel-running guard anywhere | Phase 1 (4.1) |

## Appendix B — Implementation order & effort map

Work strictly in this order; each phase leaves the tree compiling.

1. Phase 1 (Layer 0) — new files only, nothing deleted. ~6 files.
2. Phase 2 (chip drivers) — new `src/ili9341.c`, `src/xpt2046.c`; legacy files untouched. Highest-care phase: bugs #2–#5, #12–#14.
3. Phase 3 (ISR) — 3 small files.
4. Phase 4 (LVGL adapter) — port of existing logic onto new API.
5. Phase 6 CMake can be built incrementally alongside 1–4.
6. Phase 7 (tests) — write `test_ili9341_flush_async.c` FIRST and run it against the Phase 2 driver before starting Phase 4 (it validates the trickiest code).
7. Phase 8 (example restructure) — bulk `git mv`; do it in one commit separate from code changes so the diff stays reviewable.
8. Phase 9 docs last.

Definition of done: all phase acceptance criteria met, host tests green, example
builds, hardware checklist handed to the user for the final on-target pass.

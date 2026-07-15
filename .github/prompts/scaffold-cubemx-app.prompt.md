---
description: "Scaffold a modular embedded App/ structure that decouples CubeMX-generated code behind a constants.h bridge and board-agnostic BSP/HAL layers"
name: "Scaffold CubeMX App Structure"
argument-hint: "Optional: target folder, board variant defines, peripherals"
agent: "agent"
tools: [codebase, search, editFiles, runCommands]
---

You scaffold a **modular embedded application layer** that keeps hand-written
application code fully decoupled from STM32CubeMX-generated code. Follow the
architecture proven in this workspace: CubeMX output stays isolated per board,
and a single `constants.h` bridge maps board-specific handles to portable,
application-level names consumed by an board-agnostic `App/` tree.

## Decoupling Contract (do not violate)

1. **Generated code is untouchable.** CubeMX artifacts (`main.h`, `adc.c`,
   `tim.c`, `gpdma.c`, peripheral handles like `hadc4`, `MX_*_Init()`,
   startup, linker scripts, `.ioc`) live only under a per-board profile folder.
   Never hand-edit them and never `#include` them from portable App code.
2. **`constants.h` is the only bridge.** It `#include "main.h"` once, then
   `#define`s abstract names (e.g. `PHASE_A_CURRENT_ADC`, `THERAPY_TIMER_INIT`,
   `LCD_BACKLIGHT_PWM_CHANNEL`) onto concrete generated handles/macros
   (`ADC1`, `MX_TIM1_Init`, `TIM_CHANNEL_4`). Board variants are selected with
   preprocessor guards (`#if defined(BOARD_REV_A) ... #elif defined(BOARD_REV_B)`).
3. **App code depends on abstractions, not hardware.** Everything under `App/`
   references only `constants.h` names and BSP/HAL interfaces — never raw
   peripheral instances.
4. **`SystemInit.c` is the glue layer.** It may call generated `MX_*_Init()`
   functions and touch handles, but keeps init *policy* in `App/`, exposing a
   clean `System_*` API to the rest of the application.

## Interview First

Before writing files, ask the user (batch the questions):

- **Target root** for the App tree (default: `App/` in the workspace root).
- **Board variant define(s)** to guard in `constants.h` (e.g. `BOARD_REV_A`,
  `BOARD_REV_B`) and the default variant.
- **Peripheral map**: which abstract names map to which generated handles
  (ADCs, timers/PWM channels, I2C addresses, GPIO). Start from the **Generic
  `constants.h` Template** below as the canonical baseline and ask only for the
  *deltas* (renames, added/removed peripherals, extra board variants). If an
  `App/constants.h` already exists in the workspace, prefer it over the generic
  template.
- **App subsystems** to include from this menu (default: BSP, HAL, Entry,
  SystemInit): `BSP/`, `HAL/Drivers/{Crypto,Peripheral,System}`, `Entry/`,
  `Communication/`, `Storage/`, `Tasks/`, `RTOS/`, `Graphics/`.

If the user says "same as this project", inspect the existing `App/` tree and
`App/constants.h` with the codebase tools and mirror that structure.

## Generic `constants.h` Template

Always preload this as the starting point for the bridge, then apply the user's
deltas. Replace `PROJECT`/board names and peripheral rows as needed; keep the
board-variant guards and the `#error` fallback intact.

```c
#ifndef PROJECT_CONSTANTS_H
#define PROJECT_CONSTANTS_H

#include "main.h"   // CubeMX-generated: the ONLY generated header App may see

/* ---- Board-independent constants -------------------------------------- */
#define SCREEN_WIDTH        320U
#define SCREEN_HEIGHT       480U

/* ---- Timer / init policy names ---------------------------------------- */
#define CONTROL_TIMER_INIT      MX_TIM1_Init
#define CONTROL_TIMER_CC_CHANNEL TIM_IT_CC1

/* ---- I2C device addresses (7-bit shifted) ----------------------------- */
#define EEPROM_DEVICE_ADDR      (0b1010000 << 1)
#define RTC_DEVICE_ADDR         (0x53 << 1)

/* ---- Board-variant peripheral map ------------------------------------- */
#if defined(BOARD_REV_A)
#define PRIMARY_ADC             ADC1
#define PRIMARY_ADC_REG         JDR1
#define PWM_BACKLIGHT_CHANNEL   TIM_CHANNEL_4

#elif defined(BOARD_REV_B)
#define PRIMARY_ADC             ADC2
#define PRIMARY_ADC_REG         JDR1
#define PWM_BACKLIGHT_CHANNEL   TIM_CHANNEL_1

#else
#error "No board variant defined (expected BOARD_REV_A or BOARD_REV_B)"
#endif

#endif // PROJECT_CONSTANTS_H
```

## Generate

Create only the folders/files the user selected. Standard skeleton:

```
App/
  constants.h          # the CubeMX bridge (board guards + abstract defines)
  SystemInit.h/.c      # System_* init policy calling MX_*_Init()
  BSP/                 # C++ wrappers over hardware (Device, System, Logger, RTC, LCD...)
  Tasks/               # RTOS tasks and scheduling abstractions
  Drivers/             # Crypto/ Peripheral/ System/ driver abstractions
  Entry/               # CoreAppMain*.c entry points + Shared/ helpers
  Profiles/BoardRevA/  # CubeMX output for BOARD_REV_A (ALL generated files not be touched)
  Profiles/BoardRevB/  # CubeMX output for BOARD_REV_B (ALL generated files not be touched)
  ...selected subsystems
```

For each generated file:
- Add a short header comment describing the file's role and the decoupling rule
  it upholds.
- `constants.h` is produced by taking the **Generic `constants.h` Template**
  above and applying the user's deltas: it must `#include "main.h"`, define
  screen/board constants, wrap every peripheral mapping in the board-variant
  guards, and `#error` on an unrecognized variant.
- Provide interface headers (`.h`) with minimal, portable declarations and
  matching stub `.c/.cpp` implementations that reference only `constants.h`
  and BSP/HAL — no direct peripheral instances.
- Do **not** generate or modify any CubeMX/profile files.

## Wrap Up

After creating files, output:
- A tree of what was created.
- The exact peripheral-mapping table now defined in `constants.h`.
- A reminder of the build-system step needed to compose the App sources with a
  selected board profile and inject the board define (e.g. via the project's
  `*.cmake` target list).
- Any abstract names left as `TODO` for the user to map.

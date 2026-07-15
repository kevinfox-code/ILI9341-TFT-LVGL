---
description: "Scaffold a new peripheral/device driver for firmware following house style: FreeRTOS thread-safety, DMA/interrupt-driven transfers, and a documented register/command protocol."
name: "New FreeRTOS Driver"
argument-hint: "Device name + bus (e.g. 'BME280 environment sensor on I2C2') and the key registers/commands"
agent: "agent"
tools: [codebase, search, editFiles, usages]
---

# New Peripheral Driver

Generate a new device driver that is indistinguishable in structure, robustness,
elegance, and style from the existing drivers in the project's driver folder.
Use an existing register/command-protocol driver as the reference for
register/command protocol style, and an existing multi-instance bus driver as
the reference for multi-instance, interrupt/DMA-driven bus transfers. Read those
files first to match their exact conventions.

## Inputs to gather

Before writing code, confirm (ask only for what is missing from the request):

1. **Device + module prefix** — the chip/part and the short `Xyz_` function
   prefix (e.g. `ISD04_`, `SFM3200_`, `TPS050_`, `LimitSwitch_`). Files live in
   the device's own `App/Drivers/<device>/` folder as `<name>.c` / `<name>.h`.
2. **Bus + transfer mode** — SPI / I2C / USART, and whether transfers use
   interrupt (`_IT`) or DMA (`_DMA`) HAL calls. Reuse the existing bus driver's
   write/write-read helpers when one exists; only touch the HAL directly if the
   device owns its own peripheral.
3. **Protocol** — register map or command set, address/data widths, framing
   (read/write bit position, lock sequences), and any calibration or init
   sequence.
4. **Concurrency** — which threads/ISRs call the driver, and whether a single
   shared instance or an indexed array of instances is required.

## Required structure (match the house style exactly)

Produce **both** a `.h` and `.c` file. Follow these conventions precisely —
do not invent a new style.

### File & section layout
- Open with the `SPDX-License-Identifier` line, then the `File:` / `Author:` /
  `Description:` block inside the `;===` comment frame, exactly as the
  reference driver (isd04) does.
- Use the `/*________________ <Section> ___...*/` separators in the same order:
  Included Files, Global Data Referenced, Constants, Macros, Data Definitions,
  Function prototypes, Data Declarations, Code.
- Precede every function with the `;~~~` banner documenting
  `Function` / `Description` / `Parameters` / `Returns` / `Assumptions`
  (include only the sections that apply, exactly as the reference does).
- The header uses `#ifndef DRIVER_<NAME>_H` include guards and mirrors the same
  section separators with public prototypes only.

### Naming & types
- Functions: `PascalCase` with the module prefix — `Xyz_Initialize`,
  `Xyz_ReadRegister`, `Xyz_WriteRegister`, `Xyz_Ready`.
- Hungarian notation: `m_` module-static, `p` pointer, `n` numeric,
  `b` boolean, `e` enum, `pf` function pointer. Constants/macros are
  `UPPER_SNAKE_CASE`.
- Model the register/command set as a `typedef enum` in the header, one
  enumerator per register in address order, each with an aligned trailing
  `// 0xNN` address comment, terminated with a `NUM_<X>_REGISTERS` sentinel.
  Match this shape exactly:

  ```c
  typedef enum DRV_REGISTER
  {
      FAULT_STATUS,            // 0x00
      VGS_STATUS,              // 0x01
      DRIVER_CONTROL,          // 0x02
      GATE_DRIVE_HS,           // 0x03
      GATE_DRIVE_LS,           // 0x04
      OCP_CONTROL,             // 0x05
      CSA_CONTROL,             // 0x06
      NUM_DRV_REGISTERS
  } DRV_REGISTER;
  ```

- Compose each command/register word from named `UPPER_SNAKE_CASE` bit-field
  macros — **one `#define` per field**, never a single opaque hex literal.
  Head each block with a `/* <REGISTER>_ADDR Nh */` comment, and give every
  macro a trailing comment that shows both the intent *and* an ASCII bit-map
  aligned to the register's bit layout, with the field's own bits shown as their
  value and all other bit groups shown as dashes. Match this shape exactly:

  ```c
  /*  CSA_GAIN_ADDR 6h */
  #define CSA_FET              0x000  // default val  0 - - -- - - - - --
  #define CSA_VREF_DIV         0x200  // default val  - 1 - -- - - - - -- Sense amplifier reference voltage is VREF divided by 2
  #define CSA_LS_REF           0x00   //              - - 0 -- - - - - --
  #define CSA_GAIN_LVL         0x40   // 0x40 for 10  - - - 01 - - - - -- // 10 for now
  #define CSA_DIS_SEN          0x00   // not used     - - - -- 0 - - - --
  #define CSA_CAL_A            0x10   // Cal A        - - - -- - 1 - - --
  #define CSA_CAL_B            0x08   // Cal B        - - - -- - - 1 - --
  #define CSA_CAL_C            0x04   // Cal C        - - - -- - - - 1 --
  #define CSA_SEN_LVL          0x03   // default val  - - - -- - - - - 11 // Sense OCP 1 V
  ```

  OR-together the field macros into a single `<REGISTER>_DATA` word that the
  init table sends.

- Build the init/config sequence as a `static const struct` of
  `{ eRegister, nValue }` pairs named `m_InitSeq[]`, iterated by
  `Xyz_Initialize`. Match this shape exactly:

  ```c
  static const struct
  {
      DRV_REGISTER    eRegister;
      uint16_t        nValue;
  } m_InitSeq[] =
  {
      { DRIVER_CONTROL, CMD_DRIVER_CONTROL_DATA },
      { GATE_DRIVE_HS,  GATE_DRIVE_HS_DATA },
      { GATE_DRIVE_LS,  GATE_DRIVE_LS_DATA },
      { OCP_CONTROL,    CMD_OCP_DATA },
      { CSA_CONTROL,    CSA_GAIN_DATA }
  };
  ```

### Thread & memory safety (mandatory)
- Guard **every** externally reachable transaction with a mutex: create it in
  `Xyz_Initialize` with `osMutexNew`, and wrap the full transfer in
  `osMutexAcquire` / `osMutexRelease` so chip-select assert → transfer →
  de-assert is atomic (mirror the mutex use in USB_DEVICE/App/usb_com.c).
- For multi-instance drivers, keep a `static const <Peripheral>[]` description
  table plus a parallel `static <Context>[]` state array (one mutex + one
  completion semaphore + `volatile bool bError` per instance), exactly like the
  peripheral/context arrays in the reference bus driver.
- Track readiness with a `static bool m_bInitialized` and expose `Xyz_Ready()`.
- Use the CMSIS-RTOS v2 API (`osMutex*`, `osSemaphore*`, `osDelay`) and the
  FreeRTOS API (`vTaskNotifyGiveFromISR`, `xTaskGetTickCount`) directly, the
  way the existing drivers and USB layer do — this project has no bespoke RTOS
  wrapper.

### Interrupt / DMA-driven transfers & data handling
- The codebase drives transfers with the non-blocking interrupt HAL calls
  (`HAL_<BUS>_Transmit_IT` / `_TransmitReceive_IT`); use `_DMA` variants only
  when the device genuinely requires DMA. Either way, kick off the non-blocking
  transfer, then block the calling thread on a **binary semaphore**
  (`osSemaphoreAcquire`) that the HAL completion callback signals. The
  error callback sets `bError` before signalling. Register callbacks with
  `HAL_<BUS>_RegisterCallback` in the init routine.
- Treat DMA/transfer buffer pointers with care: validate `pData != nullptr`,
  handle the zero-length case as a successful no-op, and convert byte counts to
  element counts based on `DataSize` (8/16/32-bit) like the reference bus
  driver's write/write-read helpers.
- Buffers handed to DMA/interrupt engines must stay valid for the whole
  transaction — do not pass stack temporaries that go out of scope, and place
  DMA buffers so they honour the platform's cache/alignment rules. If the SoC
  uses a data cache, clean before TX and invalidate after RX; note this in a
  comment where applicable.
- Never call blocking waits from an ISR. ISR/callback context only signals the
  semaphore and sets flags.

### Robustness
- Bounds-check every register/index against the `NUM_*` sentinel and reject
  out-of-range or write-protected registers (mirror the reference driver's
  range guard).
- Return `bool` success from every fallible routine; propagate
  `bSuccess && !bError`.
- On init, retry a bounded number of times (write → calibrate → verify), and
  call the project's error handler on final failure. Log faults with the
  project's event/fault logger using a packed status struct as in the reference
  driver's fault monitor.
- Do not add speculative features, extra abstractions, or error handling for
  conditions that cannot occur. Match the reference's lean surface area.

## Output

1. Create `App/Drivers/<device>/<name>.h` and `<name>.c` in the device's own
   driver folder.
2. List any new error/event enum values or `main.h` GPIO/handle symbols the
   driver depends on, and where they must be added.
3. End with a short checklist confirming: mutex-guarded transactions,
   ISR-signalled completion semaphore, DMA/transfer buffer validity/alignment
   handled, bounds-checked register access, and banners/section separators in
   place.


## Communication Protocol Structs
1. For any command or register with multiple fields, define a struct whose
   field names match the corresponding `#define` bit-field macros. Size each
   field with `uint8_t` / `uint16_t` / `uint32_t` to match the register width,
   and add `static_assert(sizeof(struct) == <register width>)` to catch
   padding.
2. For commands sent over a framed bus, define a struct in address/command/
   data order, e.g.:

   ```c
   typedef struct __attribute__((packed))
   {
       uint8_t  ADDRESS;
       uint8_t  COMMAND;
       uint8_t  DATA;
   } spi_command_t;
   ```

   Add `static_assert(sizeof(spi_command_t) == <command width>)` to catch
   padding.
3. Mark every struct that is sent over the bus `__attribute__((packed))` so
   the compiler cannot insert padding.
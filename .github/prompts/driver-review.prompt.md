---
description: "Audit an existing firmware driver for FreeRTOS thread-safety, DMA/interrupt buffer handling, protocol correctness, and house-style conformance. Reports findings; does not rewrite unless asked."
name: "Driver Review"
argument-hint: "The driver file(s) to review (e.g. '<path>/driver-spi.c'), or leave empty for the active editor"
agent: "agent"
tools: [codebase, search, usages]
---

# Driver Review

Review the target driver against the project's house conventions and report
findings. If no file is given, review the driver currently open in the editor.
Use an existing, well-formed driver in the project — one register/command-
protocol driver and one multi-instance bus driver — as the reference for correct
style and patterns. Read the target implementation file **and** its header
before reporting.

By default this is a **read-only audit** — do not edit files. Only apply fixes
if the user explicitly asks you to.

## What to check

### 1. Thread & memory safety
- Every externally reachable transaction is guarded by a mutex
  (`osMutexAcquire` → work → `osMutexRelease`), covering the full sequence
  (e.g. chip-select assert → transfer → de-assert) with no early `return`
  between acquire and release that leaks the lock.
- Mutexes/semaphores are created in the init routine before first use.
- Multi-instance drivers use a `static const <Peripheral>[]` table + parallel
  `static <Context>[]` state array (mutex + completion semaphore +
  `volatile bool bError` per instance) rather than shared globals.
- Shared state touched from ISR/callback context is `volatile` and only
  signalled/flagged — no blocking calls from an ISR.
- Uses the CMSIS-RTOS v2 (`osMutex*`/`osSemaphore*`/`osDelay`) and FreeRTOS
  APIs directly, consistent with the existing drivers and USB layer.

### 2. Interrupt / DMA transfers & data handling
- Non-blocking transfer (`HAL_*_IT` / `HAL_*_DMA`) is paired with a binary
  semaphore wait; the completion callback signals it and the error callback sets
  `bError` before signalling.
- Callbacks are registered (`HAL_*_RegisterCallback`) during init.
- Buffer pointers are null-checked; zero-length transfers are a successful
  no-op; byte counts are converted to element counts per `DataSize`.
- DMA/transfer buffers outlive the transaction (no out-of-scope stack
  temporaries) and honour cache/alignment requirements (clean before TX,
  invalidate after RX where the SoC has a data cache).
- Return value propagates `bSuccess && !bError`.

### 3. Protocol correctness
- Register/command set modelled as a `typedef enum` with one enumerator per
  register in address order, each carrying an aligned `// 0xNN` address comment,
  and terminated with a `NUM_*` sentinel, e.g.:

  ```c
  typedef enum DRV_REGISTER
  {
      FAULT_STATUS,            // 0x00
      ...
      CSA_CONTROL,             // 0x06
      NUM_DRV_REGISTERS
  } DRV_REGISTER;
  ```

- Every register/index access is bounds-checked against the sentinel; writes to
  read-only/reserved registers are rejected.
- Command words are composed from named `UPPER_SNAKE_CASE` bit-field macros —
  **one `#define` per field**, never a bare hex literal — headed by a
  `/* <REGISTER>_ADDR Nh */` comment, with each macro carrying a trailing
  ASCII bit-map comment showing the field's bit position/value against dashes
  for the other fields, e.g.:

  ```c
  /*  CSA_GAIN_ADDR 6h */
  #define CSA_GAIN_LVL         0x40   // 0x40 for 10  - - - 01 - - - - --
  #define CSA_SEN_LVL          0x03   // default val  - - - -- - - - - 11 // Sense OCP 1 V
  ```

  Flag any register value that is a single opaque literal instead of OR-ed,
  documented field macros.
- Init/config sequence is a `static const struct { eRegister; nValue; }
  m_InitSeq[]` table iterated by the init routine, rather than a hand-unrolled
  series of writes.
- Command word framing (read/write bit, address shift, data mask) matches the
  device datasheet and the reference style; bit-field macros are named and
  commented.
- Init/calibration/verify sequence is bounded-retry and fails via the project's
  error handler; faults are logged via the project's event/fault logger.

### 4. House style & robustness
- `SPDX-License-Identifier` line and `File:` / `Author:` / `Description:`
  banner present inside the `;===` frame.
- `/*___ Section ___*/` separators in the canonical order; per-function `;~~~`
  banners with `Function` / `Description` / `Parameters` / `Returns` /
  `Assumptions`.
- Header has `#ifndef DRIVER_<NAME>_H` guards and exposes public prototypes only.
- Hungarian notation and naming prefixes are consistent; macros are
  `UPPER_SNAKE_CASE`.
- No speculative features, dead code, or error handling for impossible states.

## Output format

Report as a table, most severe first:

| Severity | Location | Finding | Suggested fix |
|----------|----------|---------|---------------|

Use `file#Lnn` locations. Severities:
- **Critical** — data race, lock leak, ISR-unsafe call, DMA buffer lifetime/
  aliasing/cache bug, or missing bounds check (memory-safety impact).
- **Major** — protocol/framing error, unpropagated error, missing completion
  sync, unchecked HAL return.
- **Minor** — style/banner/naming/section deviations, missing docs.

Finish with a one-line verdict: **ship / fix-before-merge / needs-rework**, and
list the Critical/Major items that must be resolved first. If everything passes,
say so explicitly per category rather than inventing issues.

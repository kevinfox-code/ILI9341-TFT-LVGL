---
name: review-firmware
agent: agent
description: Structured code review for STM32 firmware — checks safety, correctness, and embedded best practices
---

Review the selected code (or the current file) for the following firmware-specific concerns. For each category, list concrete findings with file and line references.

## 1. Memory Safety

- Buffer overflows: are all array accesses bounds-checked?
- Uninitialized variables: is every variable assigned before first read?
- Dangling pointers: does any pointer outlive the data it references?
- Stack usage: are there large local arrays that should be static/global?
- No dynamic allocation: confirm zero uses of `malloc`, `free`, `new`, `delete`, `calloc`, `realloc`.

## 2. Interrupt Safety

- Are all ISR-shared variables declared `volatile`?
- Are shared data accesses wrapped in critical sections (`__disable_irq()/__enable_irq()` or RTOS mutex)?
- Do ISRs avoid `HAL_Delay()`, `printf()`, floating-point, and blocking calls?
- Is each ISR short (flag + return pattern)?
- Are ISR timing budgets documented?

## 3. HAL Correctness

- Is every `HAL_*` function return value checked against `HAL_OK`?
- Are `HAL_*_Init()` calls in the correct startup sequence (clocks enabled before peripheral init)?
- Are `__HAL_RCC_*_CLK_ENABLE()` calls present before each peripheral init?
- For DMA: is alignment correct? Is cache handled (F7/H7)?

## 4. Resource Conflicts

- Are there DMA stream/channel conflicts? Cross-reference the DMA allocation table.
- Are there timer channel conflicts or pin mux collisions?
- Are alternate function mappings correct for the target MCU?

## 5. Concurrency & Synchronization

- Race conditions between ISR and main loop / RTOS tasks?
- Proper use of `volatile`, atomics, or mutexes for shared state?
- Are critical sections as short as possible?

## 6. Coding Standards

- MISRA-C: `static` for file-local symbols, `const` correctness, explicit casts, `default` in switches, `else` after `if/else if`
- Named constants for all magic numbers
- Doxygen comments on all public functions
- Function length ≤ ~50 lines

## 7. Performance

- Any unnecessary computation in ISR context?
- Are lookup tables used where appropriate instead of runtime calculation?
- Are tight loops optimized (avoid division, use bit shifts for power-of-2)?

## Output Format

For each finding, provide:
```
[CATEGORY] severity: description
  → file:line — suggested fix
```

Severity levels: 🔴 CRITICAL (must fix) | 🟡 WARNING (should fix) | 🔵 INFO (consider)

End with a summary: total findings per severity, and an overall assessment.

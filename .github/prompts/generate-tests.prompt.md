---
name: generate-tests
description: Generate unit tests for embedded firmware modules — with HAL mocking strategy
---

Generate unit tests for the specified firmware module. Tests must be runnable on a **host machine** (x86/ARM64) using a test framework (Unity, GoogleTest, or CeedlingTest as appropriate for the project).

## Strategy

### For logic-only code (no HAL dependencies)

Generate standard unit tests directly. These are straightforward — test inputs, outputs, edge cases, and error conditions.

### For HAL-dependent code

Generate tests using a **mock/stub HAL layer**:

1. Create a mock header that provides the same function signatures as the real HAL but with controllable behavior:
   ```c
   // mock_hal_uart.h
   #ifndef MOCK_HAL_UART_H
   #define MOCK_HAL_UART_H

   #include <stdint.h>
   #include <stdbool.h>

   // Mimic HAL types
   typedef enum { HAL_OK = 0, HAL_ERROR, HAL_BUSY, HAL_TIMEOUT } HAL_StatusTypeDef;

   // Mock control — set these before calling the function under test
   extern HAL_StatusTypeDef mock_uart_transmit_return;
   extern uint8_t mock_uart_tx_buffer[];
   extern uint16_t mock_uart_tx_len;

   // Mock implementation
   HAL_StatusTypeDef HAL_UART_Transmit(void *huart, const uint8_t *data, uint16_t size, uint32_t timeout);

   void mock_uart_reset(void);

   #endif
   ```

2. Implement the mock to record calls and return configurable values:
   ```c
   // mock_hal_uart.c
   HAL_StatusTypeDef mock_uart_transmit_return = HAL_OK;
   uint8_t mock_uart_tx_buffer[256];
   uint16_t mock_uart_tx_len = 0;

   HAL_StatusTypeDef HAL_UART_Transmit(void *huart, const uint8_t *data, uint16_t size, uint32_t timeout) {
       (void)huart; (void)timeout;
       memcpy(mock_uart_tx_buffer + mock_uart_tx_len, data, size);
       mock_uart_tx_len += size;
       return mock_uart_transmit_return;
   }

   void mock_uart_reset(void) {
       mock_uart_transmit_return = HAL_OK;
       mock_uart_tx_len = 0;
       memset(mock_uart_tx_buffer, 0, sizeof(mock_uart_tx_buffer));
   }
   ```

3. Use `#ifdef UNIT_TEST` or build-system linking to swap real HAL for mocks at compile time.

## Test Coverage Expectations

For each function under test, generate tests for:

- **Normal operation**: typical inputs, expected outputs
- **Boundary conditions**: zero-length buffers, maximum sizes, min/max parameter values
- **Error paths**: HAL returning `HAL_ERROR`, `HAL_BUSY`, `HAL_TIMEOUT`
- **Invalid inputs**: NULL pointers, out-of-range values (if not caught by types)
- **State transitions**: init → operational → deinit → re-init sequences

## Output Format

For each module, generate:

1. `test_<module>.c` — Test cases
2. `mock_hal_<peripheral>.h` / `mock_hal_<peripheral>.c` — Mock layer (if HAL-dependent)
3. Brief description of what each test verifies

## Naming Convention

```c
void test_<module>_<function>_<scenario>(void) {
    // Arrange
    // Act
    // Assert
}

// Examples:
void test_uart_driver_transmit_normal(void);
void test_uart_driver_transmit_hal_error_returns_failure(void);
void test_uart_driver_transmit_zero_length_returns_ok(void);
void test_uart_driver_init_called_twice_deinits_first(void);
```

# Full-System Display Tests

This folder contains tests for the consolidated display stack (`Drivers/Display`) and end-to-end validation guidance.

## Included Tests

- `validate_display_stack.py`
  - Verifies the consolidated driver files exist.
  - Verifies `CMakeLists.txt` uses `Drivers/Display` for sources and includes.
  - Verifies stale `Drivers/ILI9341` and `Drivers/XPT2046` CMake path references are removed.
- `hardware_smoke_checklist.md`
  - Board-level run checklist for display and touch runtime behavior.

## Run Structural Validation

From project root:

```sh
python3 tests/full_system/validate_display_stack.py
```

A non-zero exit code means one or more required checks failed.

## Deterministic Runtime Policy

These driver policies are controlled via CMake cache values:

- `DISPLAY_DETERMINISTIC_MODE` (default: `ON`)
- `DISPLAY_DROP_FRAME_IF_DMA_BUSY` (default: `ON`)
- `DISPLAY_SPI_TIMEOUT_MS` (default: `100`)
- `DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS` (default: `20`)
- `DISPLAY_DMA_BUSY_WAIT_SLICE_MS` (default: `1`)

Example configure command:

```sh
cmake -S . -B build/Release -D CMAKE_BUILD_TYPE=Release \
  -D DISPLAY_DETERMINISTIC_MODE=ON \
  -D DISPLAY_DROP_FRAME_IF_DMA_BUSY=ON \
  -D DISPLAY_SPI_TIMEOUT_MS=100
```

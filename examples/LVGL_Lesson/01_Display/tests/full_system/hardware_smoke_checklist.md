# Hardware Smoke Checklist (Display + Touch)

Use this checklist after flashing firmware to validate the full integrated display subsystem.

## Preconditions

- Firmware built from current main branch.
- Board mapping selected correctly via `BOARD_TARGET`.
- Display and touch hardware connected to board pins defined in `App/Board/boards/*`.

## Checks

1. Boot and initialization
- Device boots without HardFault.
- No repeated init/reset loop is observed.

2. Display output
- Backlight/display powers on.
- LVGL content renders with correct orientation.
- No persistent tearing or corrupted frame regions.

3. Touch input
- Touch press is detected on-screen.
- Drag/move actions track as expected.
- Corner taps map to expected UI locations.

4. Calibration behavior
- Run touch calibration path.
- Verify improved touch accuracy after calibration.
- Verify reset to default calibration path works.

5. Stability
- Leave GUI running for 10+ minutes.
- Verify no freeze, watchdog reset, or display DMA deadlock.

## Result

- Pass if all checks complete without failure.
- Fail if any check is unstable or incorrect; capture logs/UART traces and board target used.

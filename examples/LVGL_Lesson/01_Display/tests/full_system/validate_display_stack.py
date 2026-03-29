#!/usr/bin/env python3
"""Structural smoke test for consolidated display stack layout."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
CMAKE_FILE = ROOT / "CMakeLists.txt"

REQUIRED_DRIVER_FILES = [
    ROOT / "Drivers/Display/ILI9341.c",
    ROOT / "Drivers/Display/ILI9341.h",
    ROOT / "Drivers/Display/LCDController.c",
    ROOT / "Drivers/Display/LCDController.h",
    ROOT / "Drivers/Display/display_runtime_config.h",
    ROOT / "Drivers/Display/XPT2046.c",
    ROOT / "Drivers/Display/XPT2046.h",
    ROOT / "Drivers/Display/TouchController.c",
    ROOT / "Drivers/Display/TouchController.h",
]

REQUIRED_CMAKE_STRINGS = [
    "Drivers/Display/ILI9341.c",
    "Drivers/Display/LCDController.c",
    "Drivers/Display/XPT2046.c",
    "Drivers/Display/TouchController.c",
    "Drivers/Display",
    "DISPLAY_DETERMINISTIC_MODE",
    "DISPLAY_DROP_FRAME_IF_DMA_BUSY",
    "DISPLAY_SPI_TIMEOUT_MS",
    "DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS",
    "DISPLAY_DMA_BUSY_WAIT_SLICE_MS",
]

FORBIDDEN_CMAKE_STRINGS = [
    "Drivers/ILI9341/ILI9341.c",
    "Drivers/ILI9341/LCDController.c",
    "Drivers/XPT2046/XPT2046.c",
    "Drivers/XPT2046/TouchController.c",
    "Drivers/ILI9341",
    "Drivers/XPT2046",
]


def check_files() -> list[str]:
    failures: list[str] = []
    for file_path in REQUIRED_DRIVER_FILES:
        if not file_path.exists():
            failures.append(f"Missing required driver file: {file_path.relative_to(ROOT)}")
    return failures


def check_cmake() -> list[str]:
    failures: list[str] = []
    if not CMAKE_FILE.exists():
        return ["Missing CMakeLists.txt"]

    text = CMAKE_FILE.read_text(encoding="utf-8")

    for marker in REQUIRED_CMAKE_STRINGS:
        if marker not in text:
            failures.append(f"CMake missing required entry: {marker}")

    for marker in FORBIDDEN_CMAKE_STRINGS:
        if marker in text:
            failures.append(f"CMake still contains stale entry: {marker}")

    return failures


def main() -> int:
    failures = []
    failures.extend(check_files())
    failures.extend(check_cmake())

    if failures:
        print("Display stack validation: FAILED")
        for item in failures:
            print(f"- {item}")
        return 1

    print("Display stack validation: PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())

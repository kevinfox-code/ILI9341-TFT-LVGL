#!/usr/bin/env bash
# Configures, builds, and runs the App-level host unit tests. No ARM
# toolchain required — everything here targets the machine running this
# script.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

# A stale cache from a different source tree (e.g. the driver suite) makes
# cmake refuse to configure; start fresh in that case.
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]] \
   && ! grep -q "CMAKE_HOME_DIRECTORY:INTERNAL=${SCRIPT_DIR}$" "${BUILD_DIR}/CMakeCache.txt"; then
    rm -rf "${BUILD_DIR}"
fi

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
cmake --build "${BUILD_DIR}" -j
ctest --test-dir "${BUILD_DIR}" --output-on-failure

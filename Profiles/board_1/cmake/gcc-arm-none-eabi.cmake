set(CMAKE_SYSTEM_NAME               Generic)
set(CMAKE_SYSTEM_PROCESSOR          arm)

# Which Profiles/<name> this build targets — selects the linker script
# below. Also read by the top-level CMakeLists.txt.
if(NOT DEFINED PROFILE)
    set(PROFILE "board_1" CACHE STRING "Profiles/<PROFILE> to build against")
endif()

set(CMAKE_C_COMPILER_ID GNU)
set(CMAKE_CXX_COMPILER_ID GNU)

# Some default GCC settings
#
# Resolve arm-none-eabi-gcc explicitly rather than trusting whichever one
# PATH turns up first: on Mac this machine has three installs (Homebrew,
# STM32CubeCLT, and the STM32Cube VSCode extension's own bundle), and
# Homebrew's ships an incomplete sysroot (missing stdint.h) that silently
# poisons the CMake cache for whichever build dir gets configured while it
# wins the PATH race — including background auto-configures triggered by
# the IDE, not just interactive shells. HINTS is searched before the
# default PATH locations, so this pins the STM32CubeCLT toolchain when
# present while still falling back to PATH on other machines. STM32CLT_PATH
# is set by the Windows STM32CubeCLT installer, so this also works across
# CLT versions on Windows without hardcoding a version number.
find_program(ARM_NONE_EABI_GCC arm-none-eabi-gcc
    HINTS /opt/ST/STM32CubeCLT_1.18.0/GNU-tools-for-STM32/bin
          "$ENV{STM32CLT_PATH}/GNU-tools-for-STM32/bin"
)
get_filename_component(TOOLCHAIN_BIN_DIR ${ARM_NONE_EABI_GCC} DIRECTORY)

# Resolve each tool via find_program (not by concatenating strings) so the
# platform-correct executable suffix (".exe" on Windows, none on Mac/Linux)
# gets applied. CMAKE_C_COMPILER built as a bare path string without that
# suffix fails on Windows with "is not a full path to an existing compiler
# tool" even though the file exists as arm-none-eabi-gcc.exe.
find_program(CMAKE_C_COMPILER   arm-none-eabi-gcc     HINTS ${TOOLCHAIN_BIN_DIR} REQUIRED)
find_program(CMAKE_CXX_COMPILER arm-none-eabi-g++     HINTS ${TOOLCHAIN_BIN_DIR} REQUIRED)
find_program(CMAKE_OBJCOPY      arm-none-eabi-objcopy HINTS ${TOOLCHAIN_BIN_DIR} REQUIRED)
find_program(CMAKE_SIZE         arm-none-eabi-size    HINTS ${TOOLCHAIN_BIN_DIR} REQUIRED)

set(CMAKE_ASM_COMPILER              ${CMAKE_C_COMPILER})
set(CMAKE_LINKER                    ${CMAKE_CXX_COMPILER})

set(CMAKE_EXECUTABLE_SUFFIX_ASM     ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C       ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX     ".elf")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# MCU specific flags
set(TARGET_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard ")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-Os -g0")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -g0")

set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T \"${CMAKE_SOURCE_DIR}/Profiles/${PROFILE}/STM32F446XX_FLASH.ld\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nano.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--print-memory-usage")
set(TOOLCHAIN_LINK_LIBRARIES "m")

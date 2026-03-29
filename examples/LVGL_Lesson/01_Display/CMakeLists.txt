cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME 01_Display)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Deterministic display driver policy (recommended for control workloads)
option(DISPLAY_DETERMINISTIC_MODE "Enable deterministic display runtime policy" ON)
option(DISPLAY_DROP_FRAME_IF_DMA_BUSY "Drop LVGL frame if DMA is busy (bounded latency)" ON)
set(DISPLAY_SPI_TIMEOUT_MS "100" CACHE STRING "Blocking SPI timeout in ms for display/touch drivers")
set(DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS "20" CACHE STRING "Max wait time in ms when waiting for display DMA")
set(DISPLAY_DMA_BUSY_WAIT_SLICE_MS "1" CACHE STRING "Wait slice in ms while polling DMA busy state")

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add LVGL library with configuration
set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/Drivers/lv_conf.h")
add_subdirectory(Drivers/lvgl)
target_link_libraries(lvgl PUBLIC stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_CURRENT_LIST_DIR}/../../../Board/board_drivers.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../Display/ILI9341.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../Display/LCDController.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../Display/XPT2046.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../Display/TouchController.c
    Core/Src/cmsis_os2_ext.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_CURRENT_LIST_DIR}/../../../Board
    ${CMAKE_CURRENT_LIST_DIR}/../../../Display
    ${CMAKE_SOURCE_DIR}/Drivers/lvgl
    ${CMAKE_SOURCE_DIR}/Drivers/lvgl/src
)

# Add project symbols (macros)
# Board selection for App/Board mappings
set(BOARD_TARGET "stm32f446" CACHE STRING "Board mapping target (stm32f446|stm32f411)")
set_property(CACHE BOARD_TARGET PROPERTY STRINGS stm32f446 stm32f411)

if(BOARD_TARGET STREQUAL "stm32f446")
    set(BOARD_TARGET_DEFINE BOARD_STM32F446)
elseif(BOARD_TARGET STREQUAL "stm32f411")
    set(BOARD_TARGET_DEFINE BOARD_STM32F411)
else()
    message(FATAL_ERROR "Unsupported BOARD_TARGET='${BOARD_TARGET}'. Use stm32f446 or stm32f411.")
endif()

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    ${BOARD_TARGET_DEFINE}
    DISPLAY_DETERMINISTIC_MODE=$<IF:$<BOOL:${DISPLAY_DETERMINISTIC_MODE}>,1,0>
    DISPLAY_DROP_FRAME_IF_DMA_BUSY=$<IF:$<BOOL:${DISPLAY_DROP_FRAME_IF_DMA_BUSY}>,1,0>
    DISPLAY_SPI_TIMEOUT_MS=${DISPLAY_SPI_TIMEOUT_MS}
    DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS=${DISPLAY_DMA_BUSY_WAIT_TIMEOUT_MS}
    DISPLAY_DMA_BUSY_WAIT_SLICE_MS=${DISPLAY_DMA_BUSY_WAIT_SLICE_MS}
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl

    # Add user defined libraries
)

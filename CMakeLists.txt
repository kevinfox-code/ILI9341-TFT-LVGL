cmake_minimum_required(VERSION 3.22)
project(tft_drivers C)

# -----------------------------------------------------------------------------
# Consumer contract
#
# This library does not know your CubeMX include paths, MCU defines, or where
# drv_constants.h lives. Before add_subdirectory()'ing this repo, define an
# INTERFACE target named `drv_platform` that carries:
#   - the include directory containing your app's drv_constants.h (App/)
#   - your CubeMX Core/Inc, Drivers/CMSIS, Drivers/STM32<family>_HAL_Driver
#     include directories
#   - your CMSIS-RTOS2 / FreeRTOS include directories (when DRV_USE_CMSIS_RTOS2)
#   - MCU compile definitions (e.g. STM32F446xx, USE_HAL_DRIVER)
#
# and, if DRV_ADAPTER_LVGL is ON, make sure a target named `lvgl` already
# exists — this library links against it, it does not create it.
#
#   add_library(drv_platform INTERFACE)
#   target_include_directories(drv_platform INTERFACE App/ ...)
#   target_compile_definitions(drv_platform INTERFACE STM32F446xx USE_HAL_DRIVER)
#   add_subdirectory(path/to/ILI9341-TFT-LVGL tft_drivers)
# -----------------------------------------------------------------------------

option(DRV_USE_CMSIS_RTOS2       "Use CMSIS-RTOS2 OSAL backend (else bare-metal)" ON)
option(DRV_PROVIDE_HAL_CALLBACKS "Compile default HAL callback glue"             ON)
option(DRV_ADAPTER_LVGL          "Compile the LVGL adapter"                      OFF)
option(DRV_DROP_FRAME_IF_BUSY    "LVGL adapter drops a frame instead of blocking when flush is busy" ON)

set(DRV_SPI_TIMEOUT_MS           "100" CACHE STRING "Blocking SPI transfer timeout (ms)")
set(DRV_DMA_BUSY_WAIT_TIMEOUT_MS "200" CACHE STRING "LVGL flush busy-wait timeout (ms)")
set(DRV_SPI_MAX_BUSES            "2"   CACHE STRING "Max concurrent drv_spi_bus_t objects")
set(ILI9341_MAX_LINE_PX          "320" CACHE STRING "Max ILI9341_FillScreen line width (px)")

add_library(tft_drivers STATIC
    src/drv_setup.c
    src/ili9341.c
    src/xpt2046.c
    src/port/drv_spi_stm32.c
    src/port/drv_gpio_stm32.c
)

if(DRV_USE_CMSIS_RTOS2)
    target_sources(tft_drivers PRIVATE src/os/drv_os_cmsis2.c)
else()
    target_sources(tft_drivers PRIVATE src/os/drv_os_baremetal.c)
endif()

if(DRV_PROVIDE_HAL_CALLBACKS)
    target_sources(tft_drivers PRIVATE src/drv_isr_glue.c)
endif()

target_include_directories(tft_drivers PUBLIC include)

target_compile_definitions(tft_drivers PUBLIC
    DRV_USE_CMSIS_RTOS2=$<BOOL:${DRV_USE_CMSIS_RTOS2}>
    DRV_DROP_FRAME_IF_BUSY=$<BOOL:${DRV_DROP_FRAME_IF_BUSY}>
    DRV_SPI_TIMEOUT_MS=${DRV_SPI_TIMEOUT_MS}
    DRV_DMA_BUSY_WAIT_TIMEOUT_MS=${DRV_DMA_BUSY_WAIT_TIMEOUT_MS}
    DRV_SPI_MAX_BUSES=${DRV_SPI_MAX_BUSES}
    ILI9341_MAX_LINE_PX=${ILI9341_MAX_LINE_PX}
)

if(TARGET drv_platform)
    target_link_libraries(tft_drivers PRIVATE drv_platform)
endif()

if(DRV_ADAPTER_LVGL)
    target_sources(tft_drivers PRIVATE
        adapters/lvgl/lv_ili9341.c
        adapters/lvgl/lv_xpt2046.c
        adapters/lvgl/lv_touch_calibrate.c
    )
    target_include_directories(tft_drivers PUBLIC adapters/lvgl)
    if(TARGET lvgl)
        target_link_libraries(tft_drivers PUBLIC lvgl)
    else()
        message(WARNING "DRV_ADAPTER_LVGL=ON but no `lvgl` target exists yet; "
                         "make sure add_subdirectory(<lvgl>) runs before this one, "
                         "or define the `lvgl` target first.")
    endif()
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_layering.cmake)

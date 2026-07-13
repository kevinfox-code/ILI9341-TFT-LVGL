# App/ — application code + the tft_drivers/lvgl wiring. Never regenerated
# by CubeMX; everything CubeMX owns lives under Profiles/${PROFILE}/.

set(TFT_DRIVERS_ROOT ${CMAKE_CURRENT_LIST_DIR}/drivers)
set(PROFILE_ROOT ${CMAKE_CURRENT_LIST_DIR}/../Profiles/${PROFILE})

# ---------------------------------------------------------------------------
# drv_platform: the contract tft_drivers' CMakeLists.txt asks every consumer
# to provide (include dirs / defines for the CubeMX + CMSIS-RTOS2 headers
# App/drv_constants.h and the driver ports need, but the library itself
# must not know about).
# ---------------------------------------------------------------------------
add_library(drv_platform INTERFACE)
target_include_directories(drv_platform INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROFILE_ROOT}/Core/Inc
    ${PROFILE_ROOT}/Drivers/CMSIS/Include
    ${PROFILE_ROOT}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${PROFILE_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${PROFILE_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${PROFILE_ROOT}/Middlewares/Third_Party/FreeRTOS/Source/include
    ${PROFILE_ROOT}/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2
    ${PROFILE_ROOT}/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
)
target_compile_definitions(drv_platform INTERFACE
    STM32F446xx
    USE_HAL_DRIVER
)

# ---------------------------------------------------------------------------
# LVGL
# ---------------------------------------------------------------------------
set(LV_CONF_PATH "${CMAKE_CURRENT_LIST_DIR}/lv_conf.h")
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/lvgl lvgl)
target_link_libraries(lvgl PUBLIC drv_platform)

# ---------------------------------------------------------------------------
# tft_drivers (vendored under App/drivers/)
# ---------------------------------------------------------------------------
set(DRV_USE_CMSIS_RTOS2 ON CACHE BOOL "" FORCE)
set(DRV_ADAPTER_LVGL ON CACHE BOOL "" FORCE)
add_subdirectory(${TFT_DRIVERS_ROOT} tft_drivers)

# ---------------------------------------------------------------------------
# App sources
# ---------------------------------------------------------------------------
add_library(app STATIC
    app_main.c
    system.c
    gui/gui.c
)
target_include_directories(app PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(app PUBLIC tft_drivers lvgl drv_platform)

cmake_minimum_required(VERSION 3.22)
project(tft_drivers_host_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

find_package(Threads REQUIRED)

enable_testing()

add_library(unity STATIC vendor/unity/unity.c)
target_include_directories(unity PUBLIC vendor/unity)

# Driver sources under test are host-buildable as-is: ili9341.c/xpt2046.c
# only see drv/drv_spi.h, drv/drv_gpio.h, drv/drv_os.h — never a CubeMX or
# STM32 HAL header — so they link directly against the mocks below instead
# of src/port/*_stm32.c / src/os/drv_os_*.c.
add_library(drv_under_test STATIC
    ${REPO_ROOT}/src/ili9341.c
    ${REPO_ROOT}/src/xpt2046.c
)
target_include_directories(drv_under_test PUBLIC ${REPO_ROOT}/include)
# Host tests create many ili9341_t/xpt2046_t instances across a process
# lifetime (no Deinit exists); the driver's static instance pool is sized
# for a real embedded target (1), so widen it just for these test binaries.
target_compile_definitions(drv_under_test PUBLIC
    ILI9341_MAX_INSTANCES=32
    XPT2046_MAX_INSTANCES=32
)

add_library(mocks STATIC
    mocks/mock_seq.c
    mocks/mock_os.c
    mocks/mock_gpio.c
    mocks/mock_spi.c
)
target_include_directories(mocks PUBLIC mocks ${REPO_ROOT}/include)

function(drv_host_test name)
    add_executable(${name} ${name}.c)
    target_link_libraries(${name} PRIVATE unity drv_under_test mocks)
    target_include_directories(${name} PRIVATE mocks ${REPO_ROOT}/include)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

drv_host_test(test_ili9341_init)
drv_host_test(test_ili9341_window)
drv_host_test(test_ili9341_flush_async)
drv_host_test(test_xpt2046_read)
drv_host_test(test_xpt2046_mapping)

# The bare-metal OSAL test links the real backend directly (not the mocks
# above, which stand in for the *STM32 ports*, not for drv_os itself) plus a
# tiny wall-clock HAL_Delay/HAL_GetTick so its timeout/ISR-give behavior can
# be exercised for real.
add_executable(test_osal_baremetal
    test_osal_baremetal.c
    mocks/mock_hal_tick.c
    ${REPO_ROOT}/src/os/drv_os_baremetal.c
)
target_include_directories(test_osal_baremetal PRIVATE ${REPO_ROOT}/include)
target_link_libraries(test_osal_baremetal PRIVATE unity Threads::Threads)
add_test(NAME test_osal_baremetal COMMAND test_osal_baremetal)

# ---------------------------------------------------------------------------
# Layering check (Phase 6) — run here too so `ctest` from this directory
# also covers it.
# ---------------------------------------------------------------------------
add_test(NAME layering_check
    COMMAND ${CMAKE_COMMAND} -DROOT=${REPO_ROOT} -P ${REPO_ROOT}/cmake/layering_check_script.cmake
)

cmake_minimum_required(VERSION 3.22)
project(app_host_tests C)

# Host unit tests for the App/-level modules that are deliberately HAL-free
# (currently clock_protocol — the UART time-sync wire protocol). The driver
# library has its own suite under App/drivers/tests/host/; Unity is vendored
# there and reused here.

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(UNITY_DIR ${REPO_ROOT}/App/drivers/tests/host/vendor/unity)

enable_testing()

add_library(unity STATIC ${UNITY_DIR}/unity.c)
target_include_directories(unity PUBLIC ${UNITY_DIR})

add_library(clock_protocol STATIC ${REPO_ROOT}/App/clock_protocol.c)
target_include_directories(clock_protocol PUBLIC ${REPO_ROOT}/App)
target_compile_options(clock_protocol PRIVATE -Wall -Wextra)

add_executable(test_clock_protocol test_clock_protocol.c)
target_link_libraries(test_clock_protocol PRIVATE unity clock_protocol)
target_compile_options(test_clock_protocol PRIVATE -Wall -Wextra)
add_test(NAME test_clock_protocol COMMAND test_clock_protocol)

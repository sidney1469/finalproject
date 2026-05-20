# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(sensor_node)

target_sources(app PRIVATE
    src/main.c
    src/ble_transfer.c
    src/gps_driver.c
    src/serial.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src/protocol.c
)

target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src
)
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(base)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src)

target_sources(app PRIVATE
  src/main.c
    src/minheap.c
    src/ble_central.c
    ${LIB_DIR}/protocol.c
)

target_include_directories(app PRIVATE
  src
  ${LIB_DIR}
  ${ZEPHYR_BASE}/samples/bluetooth
)

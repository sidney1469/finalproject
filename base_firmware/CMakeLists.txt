cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(base)

target_sources(app PRIVATE
  src/main.c
  src/minheap.c
  src/ble_central.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src/protocol.c
)

zephyr_library_include_directories(${ZEPHYR_BASE}/samples/bluetooth)

target_include_directories(app PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/src
)
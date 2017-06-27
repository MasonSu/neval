cmake_minimum_required(VERSION 2.8.12)
project(neval)

file(GLOB SOURCES "src/*.c")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -Wall -Wextra -Wno-unused-function -std=c11")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(neval ${SOURCES})
add_subdirectory(tests)

install(TARGETS neval DESTINATION ${CMAKE_BINARY_DIR}/../)
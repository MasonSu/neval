cmake_minimum_required(VERSION 2.8.12)
project(neval)

file(GLOB SOURCES "src/*.c")
set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-g -Wall -Wextra -std=c11")
add_executable(neval ${SOURCES})
add_subdirectory(tests)
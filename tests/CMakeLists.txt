include_directories(../src)

add_executable(heap_test heap_test.c ../src/heap.c)

add_executable(offset_test offset_test.c)

add_executable(multiple_write_test, multiple_write_test.c)
include_directories(../src)

add_executable(heap_test heap_test.c ../src/heap.c)

add_executable(offset_test offset_test.c)

add_executable(multiple_write_test multiple_write_test.c)

add_executable(hash_test hash_test.c ../src/hash.c)

add_executable(shared_resources_test shared_resources_test.c)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS -Wall)

add_executable(memory_manager_c
        example.c
        queue.c
        memory_manager.h
        memory_manager.c)

target_link_libraries(memory_manager_c -pthread)

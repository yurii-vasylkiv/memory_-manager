set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS -Wall)

set(CMAKE_CXX_STANDARD 11)

add_executable(memory_manager_cxx
        example.cpp
        queue.cpp
        memory_manager.h
        memory_manager.cpp)

target_link_libraries(memory_manager_cxx -pthread)
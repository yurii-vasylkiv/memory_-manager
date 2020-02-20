cmake_minimum_required(VERSION 3.15)
project(memory_manager C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS -Wall)

add_executable(memory_manager main.c)
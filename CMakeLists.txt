cmake_minimum_required(VERSION 3.30)
project(PFR C)

set(CMAKE_C_STANDARD 11)

add_executable(PFR
        file_operations.c
        file_operations.h
        image_process.c
        image_process.h
)

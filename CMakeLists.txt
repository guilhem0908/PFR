cmake_minimum_required(VERSION 3.10)
project(PFR C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(pfr_core STATIC
        file_operations.c
        file_operations.h
        image_process.c
        image_process.h
        cluster.c
        cluster.h
)
target_include_directories(pfr_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(pfr_core PRIVATE -Wall -Wextra -pedantic)
endif()

add_executable(PFR main.c)
target_link_libraries(PFR PRIVATE pfr_core)

# Tests: `ctest` after the build. The regression script needs a POSIX shell.
enable_testing()

add_executable(test_units tests/test_units.c)
target_link_libraries(test_units PRIVATE pfr_core)
add_executable(gen_image tests/gen_image.c)

add_test(NAME units COMMAND test_units)
add_test(NAME regression
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.sh $<TARGET_FILE:PFR> $<TARGET_FILE:gen_image>)

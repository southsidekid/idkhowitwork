cmake_minimum_required(VERSION 3.15)
project(pingutil C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(pingutil
    src/main.c
    src/logger.c
    src/ping_sim.c
)

target_include_directories(pingutil PRIVATE include)

if (MSVC)
    target_compile_options(pingutil PRIVATE /W4)
else()
    target_compile_options(pingutil PRIVATE -Wall -Wextra -Wpedantic)
endif()


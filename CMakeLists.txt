cmake_minimum_required(VERSION 3.20)

project(mydb LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Warnings
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
)

# --------------------------------------------------
# Storage Engine Library
# --------------------------------------------------

add_library(storage
    src/storage/page.cpp
    src/storage/disk_manager.cpp
    src/storage/clock_replacer.cpp
    src/storage/buffer_pool_manager.cpp
    src/storage/heap_file.cpp
    src/storage/b_plus_tree.cpp
    src/storage/catalog.cpp
    src/storage/row.cpp
)

target_include_directories(storage
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

# --------------------------------------------------
# GoogleTest
# --------------------------------------------------

enable_testing()

include(FetchContent)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# --------------------------------------------------
# Tests
# --------------------------------------------------

add_subdirectory(tests/storage)
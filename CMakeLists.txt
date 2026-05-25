cmake_minimum_required(VERSION 3.20)
project(mydb)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# show all warnings — treat warnings as learning
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

include_directories(include)

# collect all source files automatically
file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(mydb main.cpp ${SOURCES})
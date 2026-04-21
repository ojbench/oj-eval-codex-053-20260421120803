cmake_minimum_required(VERSION 3.10)
project(oj_053)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(code main.cpp)
target_include_directories(code PRIVATE ${CMAKE_SOURCE_DIR})


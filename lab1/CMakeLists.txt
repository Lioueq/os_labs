cmake_minimum_required(VERSION 3.25)
project(lab1)

set(CMAKE_C_STANDARD 23)


add_executable(child src/child.c)
add_executable(parent src/parent.c)

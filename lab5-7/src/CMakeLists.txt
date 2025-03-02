cmake_minimum_required(VERSION 3.25)
project(lab57)

set(CMAKE_CXX_STANDARD 23)

add_executable(main_unit main_unit.cpp)
add_executable(subunit subunit.cpp)
add_library(bintree bintree.cpp)

target_link_libraries(main_unit zmq)
target_link_libraries(subunit zmq)
target_link_libraries(main_unit bintree)
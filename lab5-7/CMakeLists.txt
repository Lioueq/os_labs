cmake_minimum_required(VERSION 3.25)
project(lab57)

set(CMAKE_CXX_STANDARD 23)

set(HEADERS src/node.h src/net_func.h)


add_executable(client src/client.cpp)
add_executable(worker src/worker.cpp)

target_link_libraries(client zmq)
target_link_libraries(worker zmq)
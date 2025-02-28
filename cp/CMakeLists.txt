cmake_minimum_required(VERSION 3.25)
project(CP)

set(CMAKE_CXX_STANDARD 23)

add_executable(main src/main.cpp)

find_package(nlohmann_json 3.11.3 REQUIRED)
target_link_libraries(main PRIVATE nlohmann_json::nlohmann_json)

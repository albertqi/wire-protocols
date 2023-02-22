cmake_minimum_required(VERSION 3.17)
project(WireProtocols)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include/)

add_executable(server src/server.cpp src/network.cpp src/callback.cpp)

add_executable(client src/client.cpp src/network.cpp src/callback.cpp)

add_executable(test test/test.cpp)

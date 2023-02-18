cmake_minimum_required(VERSION 3.17)
project(WireProtocols)

set(CMAKE_CXX_STANDARD 17)

add_executable(server server.cpp network.cpp)

add_executable(client client.cpp network.cpp)

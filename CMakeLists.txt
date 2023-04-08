cmake_minimum_required(VERSION 3.17)
project(WireProtocols)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

find_package (SQLite3 REQUIRED)

include_directories(include/)
include_directories(${SQLITE3_INCLUDE_DIRS})


add_executable(server src/server.cpp src/network.cpp src/callback.cpp src/serverMain.cpp)

add_executable(client src/client.cpp src/network.cpp src/callback.cpp src/clientMain.cpp)

add_executable(test test/test.cpp src/client.cpp src/server.cpp src/network.cpp
                    src/callback.cpp)

target_link_libraries(server SQLite::SQLite3)
target_link_libraries(test SQLite::SQLite3)

cmake_minimum_required(VERSION 3.17)
project(WireProtocols)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

include_directories(include/ ${CMAKE_CURRENT_BINARY_DIR})

find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

add_library(proto proto/chat.proto)

target_link_libraries(proto PUBLIC
        protobuf::libprotobuf
        gRPC::grpc
        gRPC::grpc++
)

# Generate grpc skeleton and stub code.
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

protobuf_generate(TARGET proto LANGUAGE cpp)
protobuf_generate(TARGET proto LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

add_executable(server src/server.cpp src/network.cpp src/callback.cpp src/serverMain.cpp)

add_executable(client src/client.cpp src/network.cpp src/callback.cpp src/clientMain.cpp)

add_executable(test test/test.cpp src/client.cpp src/server.cpp src/network.cpp
                    src/callback.cpp)

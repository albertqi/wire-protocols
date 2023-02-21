# CS 262: Wire Protocols

## Build Information

This project uses CMake to generate build files. You will need cmake, make (or your build system of choice), a working c++ compiler, grpc, and protobuf installed.

Execute the following commands to compile the system:

```
mkdir build
cd build
cmake ../
make server # To compile the server
make client # To compile the client
```

## Usage Information

1. First start the server with `./server <PORT>`, where `<PORT>` is the port the server will bind to:

2. Then, start one or more clients specifiying the host address and port of the server as follows: `./client <server address> <server port>`.

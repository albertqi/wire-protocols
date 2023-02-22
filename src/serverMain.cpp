#include <iostream>

#include "server.hpp"

#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

int main(int argc, char const *argv[])
{
    Server service;

    std::string server_address = "0.0.0.0:1111";
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}

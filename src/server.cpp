#include <iostream>

#include "server.hpp"

#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

grpc::Status Server::CreateAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    return grpc::Status::OK;
}

grpc::Status Server::DeleteAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    return grpc::Status::OK;
}

grpc::Status Server::ListAccoutns(grpc::ServerContext *context, const Empty *request, ListResponse *response)
{
    return grpc::Status::OK;
}

grpc::Status Server::SendMessage(grpc::ServerContext *context, const Message *request, Response *response)
{
    return grpc::Status::OK;
}

grpc::Status Server::MessageStream(grpc::ServerContext *context, const Empty *request, grpc::ServerWriter<::Message> *writer)
{
    return grpc::Status::OK;
}

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

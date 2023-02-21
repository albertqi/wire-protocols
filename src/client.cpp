#include <string>
#include <iostream>

#include "client.hpp"

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub(ChatService::NewStub(channel))
{
    
}

int main(int argc, char const* argv[])
{
    Client client(grpc::CreateChannel("localhost:1111",
                          grpc::InsecureChannelCredentials()));

    
    return 0;
}

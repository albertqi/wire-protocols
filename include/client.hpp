#pragma once

#include <grpc/grpc.h>
#include <grpcpp/channel.h>

#include "proto/chat.grpc.pb.h"

class Client
{
public:
    Client(std::shared_ptr<grpc::Channel> channel);

private:
    std::unique_ptr<ChatService::Stub> stub;
};

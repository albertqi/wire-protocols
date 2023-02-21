#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <grpc/grpc.h>
#include <grpcpp/server.h>

#include "proto/chat.grpc.pb.h"

class Server : public ChatService::Service
{
public:

    grpc::Status CreateAccount(grpc::ServerContext* context, const Username* request, Response* response);

    grpc::Status DeleteAccount(grpc::ServerContext* context, const Username* request, Response* response);
    
    grpc::Status ListAccoutns(grpc::ServerContext* context, const Empty* request, ListResponse* response);
    
    grpc::Status SendMessage(grpc::ServerContext* context, const Message* request, Response* response);
    
    grpc::Status MessageStream(grpc::ServerContext* context, const Empty* request, grpc::ServerWriter< ::Message>* writer);
    
private:

    std::unordered_set<std::string> userList;
};

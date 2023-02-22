#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_set>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>

#include "proto/chat.grpc.pb.h"

class Client
{
public:
    Client(std::shared_ptr<grpc::Channel> channel);

    Client(std::string host, int port);

    void createAccount(std::string username);

    void getAccountList(std::string sub);

    void deleteAccount(std::string username);

private:
    std::unique_ptr<ChatService::Stub> stub;

    std::mutex m;
    std::condition_variable cv;
    std::string currentUser;
    std::unordered_set<std::string> clientUserList;
};

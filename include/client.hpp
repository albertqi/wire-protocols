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

    std::string createAccount(std::string username);

    void getAccountList(std::string sub);

    std::string deleteAccount(std::string username);

    void sendMessage(std::string recipient, std::string message);

    std::string requestMessages();

    void stopClient();

    inline std::string getCurrentUser()
    {
        return currentUser;
    }

    inline void setCurrentUser(std::string user)
    {
        currentUser = user;
    }

    inline std::unordered_set<std::string> getClientUserList()
    {
        return clientUserList;
    };

    std::atomic<bool> clientRunning;

    std::unique_ptr<ChatService::Stub> stub;

private:
    
    std::mutex m;
    std::condition_variable cv;
    std::string currentUser;
    std::unordered_set<std::string> clientUserList;
    std::string opResult;
    std::string opResultMessages;
    std::thread opThread;
};

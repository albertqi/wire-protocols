#pragma once

#include "network.hpp"
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_set>

class Client
{
public:
    Client(std::string host, int port);

   ~Client();

    Network::Message messageCallback(Network::Message message);

    Network::Message handleCreateResponse(Network::Message message);

    Network::Message handleDelete(Network::Message message);

    Network::Message handleList(Network::Message message);

    Network::Message handleReceive(Network::Message message);

    std::string createAccount(std::string username);

    std::string getAccountList(std::string sub);

    std::string deleteAccount(std::string username);

    std::string sendMessage(Network::Message message);

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

private:
    Network network;
    int clientFd;
    std::mutex m;
    std::condition_variable cv;
    std::string currentUser;
    std::unordered_set<std::string> clientUserList;
    std::string opResult;
    std::string opResultMessages;
    std::thread opThread;
};

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

    Network::Message messageCallback(Network::Message message);

    Network::Message handleCreateResponse(Network::Message message);

    Network::Message handleList(Network::Message message);

    void createAccount(std::string username);

    void getAccountList();

    void deleteAccount(std::string username);

    void stopClient();

    Network network;
    int clientFd;
    std::mutex m;
    std::condition_variable cv;
    std::string currentUser;
    std::unordered_set<std::string> clientUserList;

private:
};

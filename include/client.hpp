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

    Network::Message handleReceive(Network::Message message);

    void createAccount(std::string username);

    void getAccountList(std::string sub);

    void deleteAccount(std::string username);

    void sendMsg(Network::Message message);

    void stopClient();

    Network network;
    int clientFd;
    std::mutex m;
    std::condition_variable cv;
    std::string currentUser;
    std::unordered_set<std::string> clientUserList;

private:
};

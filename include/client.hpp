#pragma once

#include "network.hpp"

class Client
{
public:
    Client(std::string host, int port);

    Network::Message messageCallback(Network::Message message);

    Network::Message handleCreateResponse(Network::Message message);

    void createAccount(std::string username);

    void getAccountList();

    void deleteAccount(std::string username);

    Network network;
    int clientFd;

private:
};

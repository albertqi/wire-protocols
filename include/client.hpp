#pragma once

#include "network.hpp"

class Client
{
public:

    Client(std::string host, int port);

    Network::Message messageCallback(Network::Message message);

    void createAccount(std::string username);

    void getAccountList();

    Network network;
    int clientFd;


private:


};

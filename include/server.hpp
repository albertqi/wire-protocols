#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>

#include "network.hpp"

#define PORT 8080

class Server
{
public:
    Server(int port);

    ///////////////////// Server functions /////////////////////

    int acceptClient();

    void stopServer();

    //////////////////// Business functions ////////////////////

    Network::Message createAccount(Network::Message info);

    Network::Message listAccounts(Network::Message requester);

    Network::Message deleteAccount(Network::Message requester);

    Network::Message sendMessage(Network::Message message);

    Network::Message requestMessages(Network::Message message);

private:
    int serverFd;

    std::atomic<bool> serverRunning;

    std::unordered_set<std::string> userList;

    std::unordered_map<std::string, std::queue<Network::Message>> messages;

    Network network;

    int processClient(int socket);
};

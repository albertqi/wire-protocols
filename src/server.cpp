#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "server.hpp"

Server::Server(int port)
{
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        perror("socket()");
        exit(1);
    }
    int enable = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind()");
        exit(1);
    }

    if (listen(serverFd, 3) < 0)
    {
        perror("listen()");
        exit(1);
    }

    // Ignore SIGPIPE on unexpected client disconnects.
    signal(SIGPIPE, SIG_IGN);

    // Register callbacks.
    network.registerCallback(Network::CREATE, Callback(this, &Server::createAccount));
    network.registerCallback(Network::DELETE, Callback(this, &Server::deleteAccount));
    network.registerCallback(Network::SEND, Callback(this, &Server::sendMessage));
    network.registerCallback(Network::LIST, Callback(this, &Server::listAccounts));

    serverRunning = true;
}

void Server::stopServer()
{
    serverRunning = false;
}

Network::Message Server::createAccount(Network::Message info)
{
    std::string newUser = info.data;
    if (newUser.size() == 0)
    {
        return {Network::ERROR, "No username provided"};
    }

    if (userList.find(newUser) != userList.end())
    {
        return {Network::ERROR, "User already exists"};
    }

    std::cout << "Creating new account: " << newUser << "\n";
    userList.insert(newUser);

    return {Network::CREATE, newUser};
}

Network::Message Server::listAccounts(Network::Message requester)
{
    std::string result;
    std::cout << "Sending account list...\n";

    for (auto &user : this->userList)
    {
        result += user + ",";
    }

    return {Network::OK, result};
}

Network::Message Server::deleteAccount(Network::Message requester)
{
    std::string user = requester.data;

    if (userList.find(user) == userList.end())
    {
        return {Network::ERROR, "User does not exist"};
    }

    std::cout << "Deleting account: " << user << "\n";
    userList.erase(user);

    return {Network::OK};
}

Network::Message Server::sendMessage(Network::Message message)
{
    return Network::Message();
}

int Server::acceptClient()
{
    int clientSocket;
    struct sockaddr_in address;
    size_t addressLength;

    addressLength = sizeof(address);
    clientSocket = accept(serverFd, (struct sockaddr *)&address,
                          (socklen_t *)&addressLength);
    if (clientSocket < 0)
    {
        perror("accept()");
        return clientSocket;
    }

    socketThreads[clientSocket] = std::thread(&Server::processClient, this, clientSocket);

    return 0;
}

int Server::processClient(int socket)
{
    while (serverRunning)
    {
        network.receiveOperation(socket);
    }

    close(socket);

    return 0;
}

int main(int argc, char const *argv[])
{
    Server server(1111);

    while (true)
    {
        int err = server.acceptClient();
        if (err < 0)
        {
            std::cerr << "Failed client connection" << std::endl;
        }
    }

    return 0;
}

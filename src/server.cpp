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
    // Initialize socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        perror("socket()");
        exit(1);
    }
    // Enable SO_REUSEADDR so that we dont get bind() errors if the previous
    // socket is stuck in TIME_WAIT or hasn't been released by the OS.
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

    // Bind to port and mark as a listening socket.
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
    network.registerCallback(Network::REQUEST, Callback(this, &Server::requestMessages));

    serverRunning = true;
}

void Server::stopServer()
{
    serverRunning = false;
}

Network::Message Server::createAccount(Network::Message info)
{
    std::unique_lock lock(userListLock);
    std::string newUser = info.data;
    if (newUser.size() == 0)
    {
        return {Network::ERROR, "No username provided"};
    }

    if (userList.find(newUser) != userList.end())
    {
        return {Network::ERROR, "User already exists"};
    }

    std::cout << "Creating account: " << newUser << "\n";
    userList.insert(newUser);

    return {Network::CREATE, newUser};
}

Network::Message Server::listAccounts(Network::Message requester)
{
    std::unique_lock lock(userListLock);
    std::string result;
    std::string sub = requester.data;

    for (auto &user : this->userList)
    {
        if (user.find(sub) != std::string::npos)
        {
            result += user + "\n";
        }
    }

    std::cout << "Sending account list\n";
    return {Network::LIST, result};
}

Network::Message Server::deleteAccount(Network::Message requester)
{
    std::unique_lock lock(userListLock);
    std::string user = requester.data;

    if (userList.find(user) == userList.end())
    {
        return {Network::ERROR, "User does not exist"};
    }

    userList.erase(user);
    messages_lock.erase(user);

    std::cout << "Deleting account: " << user << "\n";
    return {Network::DELETE, user};
}

Network::Message Server::sendMessage(Network::Message message)
{
    std::unique_lock lock(messages_lock[message.receiver]);
    messages[message.receiver].push(message);

    std::cout << "Enqueing message from " << message.sender << " to " << message.receiver << "\n";
    return {Network::OK};
}

Network::Message Server::requestMessages(Network::Message message)
{
    std::string username = message.data;
    std::string result;

    if (username.size() <= 0 || username[0] == '\0')
    {
        return {Network::SEND, ""};;
    }

    std::unique_lock lock(messages_lock[username]);
    while (!messages[username].empty())
    {
        Network::Message msg = messages[username].front();
        std::cout << "Delivering message to " << username << "\n";
        messages[username].pop();
        result += msg.sender + ": " + msg.data + "\n";
    }

    return {Network::SEND, result};
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

    std::thread socketThread(&Server::processClient, this, clientSocket);
    socketThread.detach();

    return 0;
}

int Server::processClient(int socket)
{
    while (serverRunning)
    {
        int err = network.receiveOperation(socket);
        if (err < 0)
        {
            break;
        }
    }

    close(socket);

    return 0;
}

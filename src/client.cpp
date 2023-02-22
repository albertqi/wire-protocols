#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

#include "client.hpp"

Client::Client(std::string host, int port)
{
    // Connect to the server.
    struct sockaddr_in serverAddress;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) <= 0)
    {
        perror("inet_pton()");
        exit(1);
    }

    clientFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientFd < 0)
    {
        perror("socket()");
        exit(1);
    }

    if (connect(clientFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("connect()");
        exit(1);
    }

    // Register callbacks
    network.registerCallback(Network::OK, Callback(this, &Client::messageCallback));
    network.registerCallback(Network::CREATE, Callback(this, &Client::handleCreateResponse));
    network.registerCallback(Network::DELETE, Callback(this, &Client::handleDelete));
    network.registerCallback(Network::LIST, Callback(this, &Client::handleList));
    network.registerCallback(Network::SEND, Callback(this, &Client::handleReceive));
    network.registerCallback(Network::ERROR, Callback(this, &Client::messageCallback));

    clientRunning = true;

    // Start the receive operation thread.
    opThread = std::thread([this]()
    {
        while (clientRunning)
        {
            network.receiveOperation(clientFd);
        }
    });
}

Client::~Client()
{
    opThread.join();
}

Network::Message Client::messageCallback(Network::Message message)
{
    opResult = message.data;
    cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Client::handleCreateResponse(Network::Message message)
{
    currentUser = message.data;
    opResult = "Created account " + message.data;
    cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Client::handleDelete(Network::Message message)
{
    currentUser = "";
    opResult = "Deleted account " + message.data;
    cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Client::handleList(Network::Message message)
{
    opResult = message.data;
    clientUserList.clear();
    size_t pos = 0;
    // Split the newline seperated names into an actual list.
    while ((pos = message.data.find("\n")) != std::string::npos)
    {
        std::string user = message.data.substr(0, pos);
        if (user.size() <= 0)
        {
            break;
        }
        clientUserList.insert(user);
        message.data.erase(0, pos + 1);
    }
    cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Client::handleReceive(Network::Message message)
{
    opResultMessages = message.data;
    return {Network::NO_RETURN};
}

std::string Client::createAccount(std::string username)
{
    std::unique_lock lock(m);
    network.sendMessage(clientFd, {Network::CREATE, username});
    cv.wait(lock);
    return opResult;
}

void Client::getAccountList(std::string sub)
{
    std::unique_lock lock(m);
    network.sendMessage(clientFd, {Network::LIST, sub});
    cv.wait(lock);
}

std::string Client::deleteAccount(std::string username)
{
    std::unique_lock lock(m);
    network.sendMessage(clientFd, {Network::DELETE, username});
    cv.wait(lock);
    return opResult;
}

std::string Client::sendMessage(Network::Message message)
{
    std::unique_lock lock(m);
    network.sendMessage(clientFd, message);
    cv.wait(lock);
    return opResult;
}

std::string Client::requestMessages()
{
    network.sendMessage(clientFd, {Network::REQUEST, currentUser});
    return opResultMessages;
}

void Client::stopClient()
{
    clientRunning = false;
    close(clientFd);
}

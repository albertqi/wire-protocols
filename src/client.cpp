#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <atomic>
#include <chrono>
#include <csignal>

#include "client.hpp"

Client::Client(std::vector<std::pair<std::string, int>> serverList)
    : serverList(serverList)
{
    // Connect to the server.
    clientFd = -1;
    connectToServer(serverList);

    // Register callbacks.
    network.registerCallback(Network::OK, Callback(this, &Client::messageCallback));
    network.registerCallback(Network::CREATE, Callback(this, &Client::handleCreateResponse));
    network.registerCallback(Network::DELETE, Callback(this, &Client::handleDelete));
    network.registerCallback(Network::LIST, Callback(this, &Client::handleList));
    network.registerCallback(Network::SEND, Callback(this, &Client::handleReceive));
    network.registerCallback(Network::ERROR, Callback(this, &Client::messageCallback));

    signal(SIGPIPE, SIG_IGN);

    clientRunning = true;

    // Start the receive operation thread.
    opThread = std::thread([this]()
    {
        while (clientRunning)
        {
            if (network.receiveOperation(clientFd) < 0)
            {
                connectToServer(this->serverList);
            }
        }
    });
}

Client::~Client()
{
    opThread.join();
}

int Client::connectToServer(std::vector<std::pair<std::string, int>> serverList)
{
    std::unique_lock lk(connection_m, std::defer_lock);
    if (!lk.try_lock())
    {
        return 0;
    }

    // Close current socket before trying anything.
    if (clientFd != 0)
        close(clientFd);

    for (const auto &server : serverList)
    {
        std::string host = std::get<0>(server);
        int port = std::get<1>(server);
        struct sockaddr_in serverAddress;

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &serverAddress.sin_addr) <= 0)
        {
            continue;
        }

        clientFd = socket(AF_INET, SOCK_STREAM, 0);
        if (clientFd < 0)
        {
            continue;
        }

        if (connect(clientFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        {
            continue;
        }

        return 0;
    }

    perror("Failed to connect to server.");
    clientRunning = false;

    return -1;
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
    message_cv.notify_all();
    return {Network::NO_RETURN};
}

std::string Client::createAccount(std::string username)
{
    std::unique_lock lock(m);
    int ret = network.sendMessage(clientFd, {Network::CREATE, username});
    if (ret < 0)
    {
        connectToServer(serverList);
        return "Failed to create account. Please try again.";
    }
    cv.wait(lock);
    return opResult;
}

std::string Client::getAccountList(std::string sub)
{
    std::unique_lock lock(m);
    int ret = network.sendMessage(clientFd, {Network::LIST, sub});
    if (ret < 0)
    {
        connectToServer(serverList);
        return "Failed to get account list. Please try again.";
    }
    cv.wait(lock);
    return opResult;
}

std::string Client::deleteAccount(std::string username)
{
    std::unique_lock lock(m);
    int ret = network.sendMessage(clientFd, {Network::DELETE, username});
    if (ret < 0)
    {
        connectToServer(serverList);
        return "Failed to get delete account. Please try again.";
    }
    cv.wait(lock);
    return opResult;
}

std::string Client::sendMessage(Network::Message message)
{
    std::unique_lock lock(m);
    int ret = network.sendMessage(clientFd, message);
    if (ret < 0)
    {
        connectToServer(serverList);
        return "Failed to get send message. Please try again.";
    }
    cv.wait(lock);
    return opResult;
}

std::string Client::requestMessages()
{
    std::unique_lock lock(message_m);
    int ret = network.sendMessage(clientFd, {Network::REQUEST, currentUser});
    if (ret < 0)
    {
        connectToServer(serverList);
        return "";
    }
    using namespace std::chrono_literals;
    message_cv.wait_for(lock, 600ms);
    return opResultMessages;
}

void Client::stopClient()
{
    clientRunning = false;
    close(clientFd);
}

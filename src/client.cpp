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
}

Network::Message Client::messageCallback(Network::Message message)
{
    if (message.data.size() > 0)
    {
        opResult = message.data;
    }
    cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Client::handleCreateResponse(Network::Message message)
{
    currentUser = message.data;
    opResult = "Created account " + message.data;
    cv.notify_all();
    cv_messages.notify_all();
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
    clientUserList.clear();
    size_t pos = 0;
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

std::string Client::getAccountList(std::string sub)
{
    std::unique_lock lock(m);
    network.sendMessage(clientFd, {Network::LIST, sub});
    cv.wait(lock);
    return opResult;
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
    close(clientFd);
}

int main(int argc, char const *argv[])
{
    Client client("127.0.0.1", 1111);

    std::atomic<bool> clientRunning = true;
    std::string buffer;

    std::thread op_thread([&clientRunning, &client]()
    {
        while (clientRunning)
        {
            client.network.receiveOperation(client.clientFd);
        }
    });

    std::thread msg_thread([&clientRunning, &client]()
    {
        while (clientRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            std::string messages = client.requestMessages();
            if (messages.size() <= 0)
            {
                continue;
            }
            std::cout << "\nYou have received mail :)\n" << messages;
        }
    });

    while (clientRunning)
    {
        std::cout << "> ";
        std::getline(std::cin, buffer);
        size_t pos = buffer.find(" ");
        std::string arg1 = (pos == std::string::npos) ? buffer : buffer.substr(0, pos);
        std::string arg2 = (pos == std::string::npos) ? "" : buffer.substr(pos + 1);

        if (arg1 == "exit")
        {
            clientRunning = false;
            client.stopClient();
        }
        else if (arg1 == "login")
        {
            if (arg2.size() <= 0)
            {
                std::cout << "Please supply non-empty username" << std::endl;
                continue;
            }
            client.getAccountList("");
            if (client.clientUserList.find(arg2) == client.clientUserList.end())
            {
                std::cout << "User does not exist" << std::endl;
                continue;
            }
            client.currentUser = arg2;
            client.cv_messages.notify_all();
            std::cout << "Logged in as " << client.currentUser << std::endl;
        }
        else if (arg1 == "create")
        {
            if (arg2.size() <= 0)
            {
                std::cout << "Please supply non-empty username" << std::endl;
                continue;
            }
            std::cout << client.createAccount(arg2) << std::endl;
        }
        else if (arg1 == "delete")
        {
            if (client.currentUser.size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            std::cout << client.deleteAccount(client.currentUser) << std::endl;
        }
        else if (arg1 == "list")
        {
            client.getAccountList(arg2);

            std::string list;
            for (std::string user : client.clientUserList)
            {
                list += user + "\n";
            }
            std::cout << list;
        }
        else if (arg1 == "send")
        {
            if (client.currentUser.size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            if (arg2.size() <= 0)
            {
                std::cout << "Please supply non-empty username" << std::endl;
                continue;
            }
            client.getAccountList("");
            if (client.clientUserList.find(arg2) == client.clientUserList.end())
            {
                std::cout << "Recipient does not exist" << std::endl;
                continue;
            }
            std::string message;
            std::cout << "Enter message: ";
            std::getline(std::cin, message);
            client.sendMessage({Network::SEND, message, client.currentUser, arg2});
        }
        else if (buffer.size() > 0)
        {
            std::cout << "Invalid command" << std::endl;
        }
    }

    op_thread.join();
    msg_thread.join();
    return 0;
}

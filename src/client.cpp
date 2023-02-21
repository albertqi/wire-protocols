#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <thread>

#include "client.hpp"

std::string currentUser;

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
    network.registerCallback(Network::ERROR, Callback(this, &Client::messageCallback));
}

Network::Message Client::messageCallback(Network::Message message)
{
    std::cout << "Message recv'd: " << message.data << "\n";
    return {Network::NO_RETURN};
}

Network::Message Client::handleCreateResponse(Network::Message message)
{
    std::cout << "Message recv'd: Created account " << message.data << "\n";
    currentUser = message.data;
    return {Network::NO_RETURN};
}

void Client::createAccount(std::string username)
{
    network.sendMessage(clientFd, {Network::CREATE, username});
}

void Client::getAccountList()
{
    network.sendMessage(clientFd, {Network::LIST});
}

void Client::deleteAccount(std::string username)
{
    network.sendMessage(clientFd, {Network::DELETE, username});
}

int main(int argc, char const *argv[])
{
    Client client("127.0.0.1", 1111);

    client.createAccount("testUser");
    client.createAccount("testUser2");
    client.getAccountList();
    client.deleteAccount("testUser2");
    client.getAccountList();

    std::atomic<bool> clientRunning = true;
    std::string buffer;

    std::thread t([&clientRunning](Client client)
                  { while (clientRunning){
                    client.network.receiveOperation(client.clientFd);
                    } },
                  client);

    while (clientRunning)
    {
        std::cin >> buffer;
        if (buffer == "exit")
        {
            clientRunning = false;
        }
        else if (buffer == "login")
        {
            std::cin >> buffer;
            std::cout << buffer << std::endl;
        }
        else if (buffer == "create")
        {
            std::cin >> buffer;
            client.createAccount(buffer);
        }
        else if (buffer == "delete")
        {
            client.deleteAccount(currentUser);
        }
        else if (buffer == "list")
        {
            client.getAccountList();
        }
    }

    t.join();
    return 0;
}

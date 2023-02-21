#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

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
    network.registerCallback(Network::ERROR, Callback(this, &Client::messageCallback));
}

Network::Message Client::messageCallback(Network::Message message)
{
    std::cout << "Message recv'd: " << message.data << "\n";
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

    while (true)
    {
        client.network.receiveOperation(client.clientFd);
    }

    return 0;
}

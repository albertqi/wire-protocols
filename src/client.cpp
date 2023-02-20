#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include "network.hpp"

#define PORT 8080
 
int callback_test(Network::Message message)
{
    std::cout << "Message:\n" << message.data << "\n";
}

int main(int argc, char const* argv[])
{
    int sock = 0, valread, client_fd;
    struct sockaddr_in serv_addr;
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket()");
        return -1;
    }
 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
 
    // Convert IPv4 and IPv6 addresses from text to binary
    // form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)
        <= 0) {
        perror("inet_pton()");
        return -1;
    }
 
    client_fd = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (client_fd < 0) {
        perror("connect()");
        return -1;
    }
    
    Network network;
    network.register_callback(Network::OpCode::LIST, callback_test);
 
    while (1)
    {
    }
}
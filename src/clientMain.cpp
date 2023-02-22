#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "client.hpp"

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

int main(int argc, char const *argv[])
{
    Client client(grpc::CreateChannel("localhost:1111",
                          grpc::InsecureChannelCredentials()));

    std::thread msg_thread([&client]()
    {
        while (client.clientRunning)
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

    std::string buffer;
    while (client.clientRunning)
    {
        std::cout << "> ";
        std::getline(std::cin, buffer);
        size_t pos = buffer.find(" ");
        std::string arg1 = (pos == std::string::npos) ? buffer : buffer.substr(0, pos);
        std::string arg2 = (pos == std::string::npos) ? "" : buffer.substr(pos + 1);

        if (arg1 == "exit")
        {
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
            if (client.getClientUserList().find(arg2) == client.getClientUserList().end())
            {
                std::cout << "User does not exist" << std::endl;
                continue;
            }
            client.setCurrentUser(arg2);
            std::cout << "Logged in as " << client.getCurrentUser() << std::endl;
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
            if (client.getCurrentUser().size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            std::cout << client.deleteAccount(client.getCurrentUser()) << std::endl;
        }
        else if (arg1 == "list")
        {
            client.getAccountList(arg2);

            std::string list;
            for (std::string user : client.getClientUserList())
            {
                list += user + "\n";
            }
            std::cout << list;
        }
        else if (arg1 == "send")
        {
            if (client.getCurrentUser().size() <= 0)
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
            if (client.getClientUserList().find(arg2) == client.getClientUserList().end())
            {
                std::cout << "Recipient does not exist" << std::endl;
                continue;
            }
            std::string message;
            std::cout << "Enter message: ";
            std::getline(std::cin, message);
            client.sendMessage(message, arg2);
        }
        else if (buffer.size() > 0)
        {
            std::cout << "Invalid command" << std::endl;
        }
    }

    msg_thread.join();
    return 0;
}

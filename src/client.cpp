#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "client.hpp"

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub(ChatService::NewStub(channel))
{
}

void Client::createAccount(std::string username)
{
    Username username_obj;
    username_obj.set_name(username);

    stub.CreateAccount(username_obj);
}

void Client::getAccountList(std::string sub)
{
    std::string message = stub.ListAccounts();

    size_t pos = 0;
    while ((pos = message.find("\n")) != std::string::npos)
    {
        std::string user = message.substr(0, pos);
        if (user.size() <= 0)
        {
            break;
        }
        clientUserList.insert(user);
        message.data.erase(0, pos + 1);
    }
    cv.notify_all();
}

void Client::deleteAccount(std::string username)
{
    Username username_obj;
    username_obj.set_name(username);
    
    stub.DeleteAccount(username_obj);
}

void Client::sendMsg(std::string recipient, std::string message)
{
    Message message_obj;
    message_obj.set_sender(currentUser);
    message_obj.set_receiver(recipient);
    message_obj.set_message(message);

    stub.sendMessage(message_obj);
}

int main(int argc, char const *argv[])
{
    Client client(grpc::CreateChannel("localhost:1111",
                          grpc::InsecureChannelCredentials()));

    std::atomic<bool> clientRunning = true;
    std::string buffer;

    std::thread op_thread([&clientRunning, &client]() {
        while (clientRunning) {
            client.network.receiveOperation(client.clientFd);
        }
    });

    std::thread msg_thread([&clientRunning, &client]() {
        while (clientRunning) {
            client.network.sendMessage(client.clientFd, {Network::REQUEST, client.currentUser});
            std::this_thread::sleep_for(std::chrono::seconds(2));
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
        }
        else if (arg1 == "login")
        {
            if (arg2.size() <= 0)
            {
                std::cout << "Please supply non-empty username" << std::endl;
                continue;
            }
            std::unique_lock lock(client.m);
            client.getAccountList("");
            client.cv.wait(lock);
            if (client.clientUserList.find(arg2) == client.clientUserList.end())
            {
                std::cout << "User does not exist" << std::endl;
                continue;
            }
            client.currentUser = arg2;
            std::cout << "Logged in as " << client.currentUser << std::endl;
        }
        else if (arg1 == "create")
        {
            if (arg2.size() <= 0)
            {
                std::cout << "Please supply non-empty username" << std::endl;
                continue;
            }
            client.createAccount(arg2);
        }
        else if (arg1 == "delete")
        {
            if (client.currentUser.size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            client.deleteAccount(client.currentUser);
        }
        else if (arg1 == "list")
        {
            std::unique_lock lock(client.m);
            client.getAccountList(arg2);
            client.cv.wait(lock);

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
            std::unique_lock lock(client.m);
            client.getAccountList("");
            client.cv.wait(lock);
            if (client.clientUserList.find(arg2) == client.clientUserList.end())
            {
                std::cout << "Recipient does not exist" << std::endl;
                continue;
            }
            std::string message;
            std::cout << "Enter message: ";
            std::getline(std::cin, message);
            client.sendMsg({Network::SEND, message, client.currentUser, arg2});
        }
        else if (arg1.size() > 0)
        {
            std::cout << "Invalid command" << std::endl;
        }
    }

    op_thread.join();
    msg_thread.join();
    return 0;
}

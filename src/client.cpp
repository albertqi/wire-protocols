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

void Client::getAccountList()
{
    stub.ListAccounts();
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

    std::thread op_thread([&clientRunning, &client]()
                          { while (clientRunning){
                    client.network.receiveOperation(client.clientFd);
                    } });

    std::thread msg_thread([&clientRunning, &client]()
                           {
                                       while (clientRunning){
                               client.network.sendMessage(client.clientFd, {Network::REQUEST, client.currentUser});
                               std::this_thread::sleep_for(std::chrono::seconds(2));
                               } });

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
            std::unique_lock lock(client.m);
            client.getAccountList();
            client.cv.wait(lock);
            if (client.clientUserList.find(buffer) == client.clientUserList.end())
            {
                std::cout << "User does not exist" << std::endl;
                continue;
            }
            client.currentUser = buffer;
            std::cout << "Logged in as " << client.currentUser << std::endl;
        }
        else if (buffer == "create")
        {
            std::cin >> buffer;
            client.createAccount(buffer);
        }
        else if (buffer == "delete")
        {
            if (client.currentUser.size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            client.deleteAccount(client.currentUser);
        }
        else if (buffer == "list")
        {
            std::unique_lock lock(client.m);
            client.getAccountList();
            client.cv.wait(lock);

            std::string list;
            for (std::string user : client.clientUserList)
            {
                list += user + "\n";
            }
            std::cout << list;
        }
        else if (buffer == "send")
        {
            std::cin >> buffer;
            if (client.currentUser.size() <= 0)
            {
                std::cout << "Not logged in" << std::endl;
                continue;
            }
            std::unique_lock lock(client.m);
            client.getAccountList();
            client.cv.wait(lock);
            if (client.clientUserList.find(buffer) == client.clientUserList.end())
            {
                std::cout << "Recipient does not exist" << std::endl;
                continue;
            }
            std::string message;
            std::cout << "Send message: ";
            std::cin.ignore();
            std::getline(std::cin, message);
            client.sendMsg({Network::SEND, message, client.currentUser, buffer});
        }
    }

    op_thread.join();
    msg_thread.join();
    return 0;
}

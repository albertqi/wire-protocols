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
    grpc::ClientContext context;

    Username username_obj;
    username_obj.set_name(username);

    Response response;

    grpc::Status err = stub->CreateAccount(&context, username_obj, &response);
}

void Client::getAccountList(std::string sub)
{
    grpc::ClientContext context;

    ListQuery query;
    query.set_query(sub);

    ListResponse response;

    grpc::Status err = stub->ListAccounts(&context, query, &response);

    clientUserList.clear();
    for (const Username& user: response.accounts())
    {
        clientUserList.insert(user.name());
    }

    cv.notify_all();
}

void Client::deleteAccount(std::string username)
{
    grpc::ClientContext context;

    Username username_obj;
    username_obj.set_name(username);

    Response response;
    
    stub->DeleteAccount(&context, username_obj, &response);
}

void Client::sendMsg(std::string recipient, std::string message)
{
    grpc::ClientContext context;

    Message message_obj;
    Username* sender = new Username;
    sender->set_name(currentUser);
    Username* receiver = new Username;
    receiver->set_name(recipient);

    message_obj.set_allocated_sender (sender);
    message_obj.set_allocated_receiver(receiver);
    message_obj.set_message(message);

    Response response;

    grpc::Status err = stub->SendMessage(&context, message_obj, &response);
}

int main(int argc, char const *argv[])
{
    Client client(grpc::CreateChannel("localhost:1111",
                          grpc::InsecureChannelCredentials()));

    std::atomic<bool> clientRunning = true;
    std::string buffer;

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

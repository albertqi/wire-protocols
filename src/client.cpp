#include <atomic>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "client.hpp"

Client::Client(std::shared_ptr<grpc::Channel> channel)
    : stub(ChatService::NewStub(channel))
{
}

std::string Client::createAccount(std::string username)
{
    grpc::ClientContext context;

    Username username_obj;
    username_obj.set_name(username);

    Response response;

    grpc::Status err = stub->CreateAccount(&context, username_obj, &response);

    return response.response();
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

std::string Client::deleteAccount(std::string username)
{
    grpc::ClientContext context;

    Username username_obj;
    username_obj.set_name(username);

    Response response;
    
    stub->DeleteAccount(&context, username_obj, &response);

    return response.response();
}

void Client::sendMessage(std::string recipient, std::string message)
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

std::string Client::requestMessages()
{
    grpc::ClientContext context;

    Username user;
    user.set_name(currentUser);
    std::shared_ptr<grpc::ClientReader<Message>> stream(stub->MessageStream(&context, user));

    Message message;
    stream->Read(&message);

    return message.sender().name() + ": " + message.message();
}

void Client::stopClient()
{
    clientRunning = false;
}

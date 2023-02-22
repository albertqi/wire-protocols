#include <iostream>

#include "server.hpp"

#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

grpc::Status Server::CreateAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    std::string newUser = info.data;
    if (newUser.size() == 0)
    {
        return grpc::Status(-1, "No username provided");
    }

    if (userList.find(newUser) != userList.end())
    {
        return grpc::Status(-1, "User already exists");
    }

    std::cout << "Creating new account: " << newUser << "\n";
    userList.insert(newUser);

    return grpc::Status::OK;
}

grpc::Status Server::DeleteAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    std::string user = requester.data;

    if (userList.find(user) == userList.end())
    {
        return grpc::Status(-1, "User does not exist")
    }

    std::cout << "Deleting account: " << user << "\n";
    userList.erase(user);

    return grpc::Status::OK;
}

grpc::Status Server::ListAccoutns(grpc::ServerContext *context, const ListQuery *request, ListResponse *response)
{
    std::string result;
    std::string sub = requester.data;
    std::cout << "Sending account list...\n";

    for (auto &user : this->userList)
    {
        if (user.find(sub) != std::string::npos)
        {
            result += user + "\n";
        }
    }

    return grpc::Status::OK;
}

grpc::Status Server::SendMessage(grpc::ServerContext *context, const Message *request, Response *response)
{
    messages[message.receiver].push(message);

    return grpc::Status::OK;
}

grpc::Status Server::MessageStream(grpc::ServerContext *context, const Empty *request, grpc::ServerWriter<::Message> *writer)
{
    std::string username = message.data;

    std::string result;
    std::cout << "Sending messages for " << username << "...\n";

    while (!messages[username].empty())
    {
        Network::Message msg = messages[username].front();
        messages[username].pop();
        result += msg.sender + ": " + msg.data + "\n";
    }

    return grpc::Status::OK;
}

int main(int argc, char const *argv[])
{
    Server service;

    std::string server_address = "0.0.0.0:1111";
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
}

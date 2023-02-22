#include <iostream>

#include "server.hpp"

grpc::Status Server::CreateAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    std::string newUser = request->name();
    if (newUser.size() == 0)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "No username provided");
    }

    if (userList.find(newUser) != userList.end())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "User already exists");
    }

    std::cout << "Creating new account: " << newUser << "\n";
    userList.insert(newUser);

    return grpc::Status::OK;
}

grpc::Status Server::DeleteAccount(grpc::ServerContext *context, const Username *request, Response *response)
{
    std::string user = request->name();

    if (userList.find(user) == userList.end())
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "User does not exist");
    }

    std::cout << "Deleting account: " << user << "\n";
    userList.erase(user);

    return grpc::Status::OK;
}

grpc::Status Server::ListAccounts(grpc::ServerContext *context, const ListQuery *request, ListResponse *response)
{
    std::string result;
    std::string sub = request->query();
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
    Message* request_copy = new Message(*request);
    messages[request->receiver().name()].push(request_copy);

    return grpc::Status::OK;
}

grpc::Status Server::MessageStream(grpc::ServerContext *context, const Username *request, grpc::ServerWriter<::Message> *writer)
{
    std::string username = request->name();

    std::string result;
    std::cout << "Sending messages for " << username << "...\n";

    while (!messages[username].empty())
    {
        Message* msg = messages[username].front();
        messages[username].pop();
        writer->Write(*msg);
        delete msg;
    }

    return grpc::Status::OK;
}

#include "network.hpp"

Callback::Callback(Server* instance, Network::Message (Server::*func)(Network::Message))
{
    isClientCallback = false;
    serverCallback = func;
}

Callback::Callback(Client* instance, Network::Message (Client::*func)(Network::Message))
{
    isClientCallback = true;
    clientCallback = func;
}

Network::Message Callback::operator()(Network::Message message)
{
    if (isClientCallback)
    {
        return (client->*clientCallback)(message);
    }
    else
    {
        return (server->*serverCallback)(message);
    }
}

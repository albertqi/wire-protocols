/**
 * `Server` is the backbone of the chat service and handles all requests from
 * clients. This class keeps track of user accounts, backlogged messages, and 
 * sends messages to users on demand.
 * 
 * This class uses the `Network` class to handle parsing the wire protocol, and
 * registers each function as a callback for the various operation it chooses
 * to handle.
*/

#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#include <grpc/grpc.h>
#include <grpcpp/server.h>

#include "proto/chat.grpc.pb.h"

class Server : public ChatService::Service
{
public:

    /**
     * Creates an account using the data in `info`.
    */
    grpc::Status CreateAccount(grpc::ServerContext* context, const Username* request, Response* response);

    /**
     * Deletes the account specified by `requester`.
    */
    grpc::Status DeleteAccount(grpc::ServerContext* context, const Username* request, Response* response);
    
    /**
     * Returns a list of users. This list can be searched by substring using
     * the `data` field of `requester`.
    */
    grpc::Status ListAccounts(grpc::ServerContext* context, const ListQuery* request, ListResponse* response);
    
    /**
     * Sends the message specified by `message` to a recepient. If the recepient
     * is not currently logged in, the message is backlogged and delivered when
     * they logged in.
    */
    grpc::Status SendMessage(grpc::ServerContext* context, const Message* request, Response* response);
    
    /**
     * Returns the next message for the user specified in `requester`.
    */
    grpc::Status MessageStream(grpc::ServerContext* context, const Username* request, grpc::ServerWriter< ::Message>* writer);
    
private:

    /**
     * Stores the list of user accounts.
    */
    std::unordered_set<std::string> userList;

    /**
     * Stores the undelivered messages for each user.
    */
    std::unordered_map<std::string, std::queue<Message*>> messages;
};

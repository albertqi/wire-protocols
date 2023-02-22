/**
 * `Client` handles the client-server communication and maintians the current
 * state of a particular client (i.e., logging in and chanigng accounts). This
 * class uses the `Network` class to handle the data link layer and registers
 * callbacks for messages received from the server.
*/
#pragma once

#include <condition_variable>
#include <mutex>
#include <string>
#include <unordered_set>
#include <thread>

#include <grpc/grpc.h>
#include <grpcpp/channel.h>

#include "proto/chat.grpc.pb.h"

class Client
{
public:
    Client(std::shared_ptr<grpc::Channel> channel);

    /**
     * Creates an account on the server.
    */
    std::string createAccount(std::string username);

    /**
     * Updates the internal account list from the server.
    */
    void getAccountList(std::string sub);

    /**
     * Deletes the specified account on the server.
    */
    std::string deleteAccount(std::string username);

    /**
     * Sends the message specified in message to a recepient.
    */
    void sendMessage(std::string recipient, std::string message);

    /**
     * Retreives the next message for the currently logged in user. Returns
     * an empty string if there are no messages to receive.
    */
    std::string requestMessages();

    /**
     * Closes the client connection and cleans up resources.
    */
    void stopClient();

    //////////////////// Accessors ////////////////////

    inline std::string getCurrentUser()
    {
        return currentUser;
    }

    inline void setCurrentUser(std::string user)
    {
        currentUser = user;
    }

    inline std::unordered_set<std::string> getClientUserList()
    {
        return clientUserList;
    };

    std::atomic<bool> clientRunning;

    std::unique_ptr<ChatService::Stub> stub;

private:
    
    /**
     * Locks operation return values so handlers can communicate with calling
     * function.
    */
    std::mutex m;
    std::condition_variable cv;

    /**
     * The currently logged in user.
    */
    std::string currentUser;

    /**
     * Internal user list that is received from server.
    */
    std::unordered_set<std::string> clientUserList;

    /**
     * Return values from callbacks
    */
    std::string opResult;
    std::string opResultMessages;

    /**
     * Network `receiveOperation()` thread.
    */
    std::thread opThread;
};

/**
 * `Client` handles the client-server communication and maintains the current
 * state of a particular client (i.e., logging in and changing accounts). This
 * class uses the `Network` class to handle the data link layer and registers
 * callbacks for messages received from the server.
 */

#pragma once

#include "network.hpp"
#include <mutex>
#include <condition_variable>
#include <string>
#include <unordered_set>
#include <thread>
#include <vector>

class Client
{
public:
    Client(std::vector<std::pair<std::string, int>> serverList);

    ~Client();

    /**
     * Creates an account on the server.
     */
    std::string createAccount(std::string username);

    /**
     * Updates the internal account list from the server.
     */
    std::string getAccountList(std::string sub);

    /**
     * Deletes the specified account on the server.
     */
    std::string deleteAccount(std::string username);

    /**
     * Sends the message specified in message to a recipient.
     */
    std::string sendMessage(Network::Message message);

    /**
     * Retrieves the next message for the currently logged in user. Returns
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

    ////////// FOR TESTING PURPOSES ONLY. DO NOT USE IN PRODUCTION //////////

    /**
     * `OK` and `ERROR` handler.
     */
    Network::Message messageCallback(Network::Message message);

    /**
     * `CREATE` handler.
     */
    Network::Message handleCreateResponse(Network::Message message);

    /**
     * `DELETE` handler.
     */
    Network::Message handleDelete(Network::Message message);

    /**
     * `LIST` handler.
     */
    Network::Message handleList(Network::Message message);

    /**
     * `REQUEST` handler.
     */
    Network::Message handleReceive(Network::Message message);

private:
    /**
     * Network instance acting as data-link layer.
     */
    Network network;

    /**
     * Client socket that is connected to the server.
     */
    int clientFd;

    /**
     * Locks operation return values so handlers can communicate with calling
     * function.
     */
    std::mutex m;
    std::condition_variable cv;

    /**
     * Message return lock and condition variable.
     */
    std::mutex message_m;
    std::condition_variable message_cv;

    /**
     * Server list and lock.
     */
    std::mutex connection_m;
    std::vector<std::pair<std::string, int>> serverList;

    /**
     * The currently logged in user.
     */
    std::string currentUser;

    /**
     * Internal user list that is received from server.
     */
    std::unordered_set<std::string> clientUserList;

    /**
     * Return values from callbacks.
     */
    std::string opResult;
    std::string opResultMessages;

    /**
     * Network `receiveOperation()` thread.
     */
    std::thread opThread;

    /**
     * Connects to a server in the server list.
     */
    int connectToServer(std::vector<std::pair<std::string, int>> serverList);
};
/**
 * `Server` is the backbone of the chat service and handles all requests from
 * clients. This class keeps track of user accounts, backlogged messages, and
 * sends messages to users on demand.
 *
 * This class uses the `Network` class to handle parsing the wire protocol and
 * registers each function as a callback for the various operation it chooses
 * to handle.
 */

#pragma once

#include <atomic>
#include <thread>
#include <string>
#include <sqlite3.h>

#include "network.hpp"

#define PORT 8080

class Server
{
public:
    Server(int port);

    ///////////////////// Server functions /////////////////////

    /**
     * Accepts a client connection and spawns a thread to handle requests for
     * that client. This function returns once the connection has been accepted
     * and the thread has been spawned. The thread will terminate itself once
     * the client disconnects.
     */
    int acceptClient();

    /**
     * Closes the listening socket and cleans up resources.
     */
    void stopServer();

    //////////////////// Business functions ////////////////////

    /**
     * Creates an account using the data in `info`.
     */
    Network::Message createAccount(Network::Message info);

    /**
     * Returns a list of users. This list can be searched by substring using
     * the `data` field of `requester`.
     */
    Network::Message listAccounts(Network::Message requester);

    /**
     * Deletes the account specified by `requester`.
     */
    Network::Message deleteAccount(Network::Message requester);

    /**
     * Sends the message specified by `message` to a recepient. If the recepient
     * is not currently logged in, the message is backlogged and delivered when
     * they logged in.
     */
    Network::Message sendMessage(Network::Message message);

    /**
     * Returns the next message for the user specified in `requester`.
     */
    Network::Message requestMessages(Network::Message requester);

private:
    /**
     * Listening socket.
     */
    int serverFd;

    /**
     * Controls how long to run client processing threads.
     */
    std::atomic<bool> serverRunning;

    /**
     * Stores the database connection.
     */
    sqlite3 *db;

    /**
     * The network instance acting as the data-link layer.
     */
    Network network;

    /**
     * Thread function that is spawned to handle each client connection.
     */
    int processClient(int socket);
};

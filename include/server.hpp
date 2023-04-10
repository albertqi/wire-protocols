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
#include <mutex>
#include <condition_variable>
#include <string>
#include <thread>
#include <set>
#include <vector>

#include <sqlite3.h>

#include "network.hpp"

class Server
{
public:
    Server(int port, int replicaId, std::vector<std::pair<std::string, int>> replica_addrs);

    ~Server();

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

    int syncDatabases(int port);

private:
    /**
     * Listening socket.
     */
    int serverFd;

    /**
     * The ID of this instance in relation to the other replicas.
     */
    int replicaId;

    /**
     * Flags whether this instance is the leader of the replicas.
     */
    bool isLeader;

    /**
     * Controls how long to run client processing threads.
     */
    std::atomic<bool> serverRunning;

    /**
     * List of sockets to the other replicas.
     */
    std::vector<int> replicas;

    /**
     * Stores the database connection.
     */
    sqlite3 *db;

    /**
     * The network instance acting as the data-link layer.
     */
    Network network;

    int leaderId;

    /**
     * List of currently acive replicas determined through the IDENTIFY protocol.
     */
    std::mutex replicas_m;
    std::set<int> runningReplicas;

    /**
     * List of the last modified times of the databases of each replica.
     */
    std::vector<size_t> dbModifiedTimes;
    std::mutex db_m;
    std::condition_variable db_cv;
    std::string dbSyncedStr;
    std::mutex dbSync_m;

    /**
     * Thread function that is spawned to handle each client connection.
     */
    int processClient(int socket);

    /**
     * Forwards `message` to the other replicas if this instance is the leader.
     */
    void doReplication(Network::Message message);

    /**
     * 
     */
    void doLeaderElection();

    void discoverReplicas();

    Network::Message handleLeader(Network::Message id);

    Network::Message handleIdentify(Network::Message id);

    Network::Message handleTime(Network::Message message);

    Network::Message handleSync(Network::Message message);
};

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sqlite3.h>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <thread>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "server.hpp"

Server::Server(int port, int replicaId, std::vector<std::pair<std::string, int>> replica_addrs)
    : replicaId(replicaId)
{
    network.dropServerResponses = true;
    network.isServer = true;
    serverRunning = false;

    // Initialize socket.
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        perror("socket()");
        exit(1);
    }

    // Enable SO_REUSEADDR so that we do not get bind() errors if the previous
    // socket is stuck in TIME_WAIT or has not been released by the OS.
    int enable = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt()");
        exit(1);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind to port and mark as a listening socket.
    if (bind(serverFd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind()");
        exit(1);
    }

    if (listen(serverFd, 3) < 0)
    {
        perror("listen()");
        exit(1);
    }

    // Ignore SIGPIPE on unexpected client disconnects.
    signal(SIGPIPE, SIG_IGN);

    // Connect to other server replicas.
    for (auto server : replica_addrs)
    {
        struct sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(server.second);
        if (inet_pton(AF_INET, server.first.c_str(), &serverAddress.sin_addr) <= 0)
        {
            perror("inet_pton()");
            exit(1);
        }

        // Keep trying to connect until successful.
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        while (connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        {
            // Initialize new socket to talk to replicas.
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (serverFd < 0)
            {
                perror("socket()");
                exit(1);
            }
            std::cout << "Re-trying connection to replica..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        replicas.push_back(sock);
    }

    // First leader is just server 0.
    isLeader = replicaId == 0;
    network.isServer = !isLeader;
    leaderId = 0;

    // Register callbacks.
    network.registerCallback(Network::LEADER, Callback(this, &Server::handleLeader));
    network.registerCallback(Network::IDENTIFY, Callback(this, &Server::handleIdentify));
    network.registerCallback(Network::CREATE, Callback(this, &Server::createAccount));
    network.registerCallback(Network::DELETE, Callback(this, &Server::deleteAccount));
    network.registerCallback(Network::SEND, Callback(this, &Server::sendMessage));
    network.registerCallback(Network::LIST, Callback(this, &Server::listAccounts));
    network.registerCallback(Network::REQUEST, Callback(this, &Server::requestMessages));
    network.registerCallback(Network::TIME, Callback(this, &Server::handleTime));
    network.registerCallback(Network::SYNC, Callback(this, &Server::handleSync));

    serverRunning = true;
}

Server::~Server()
{
    stopServer();
}

void Server::stopServer()
{
    sqlite3_close(db);
    serverRunning = false;
}

int Server::syncDatabases(int port)
{
    std::unique_lock lock(db_m);

    // Find last modified time of database.
    std::string dbName = "server_" + std::to_string(port) + ".db";
    std::string msgData = "0";
    if (std::filesystem::exists(dbName))
    {
        auto lastWriteTime = std::filesystem::last_write_time(dbName);
        msgData = std::to_string((size_t)std::chrono::duration_cast<std::chrono::milliseconds>(lastWriteTime.time_since_epoch()).count());
    }

    // Send last modified time to replicas.
    for (auto socket : replicas)
    {
        if (network.sendMessage(socket, {Network::TIME, msgData}) < 0)
        {
            return -1;
        }
    }

    while (dbModifiedTimes.size() != replicas.size())
    {
        // Wait for all replicas to respond.
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    // Find last modified time of most recent database.
    size_t modifiedTime = std::stoull(msgData);
    size_t mostRecentTime = modifiedTime;
    for (auto time : dbModifiedTimes)
    {
        if (time > mostRecentTime)
        {
            mostRecentTime = time;
        }
    }

    // Check if there exists at least one replica with a database.
    if (mostRecentTime != 0)
    {
        // Check if we need to sync.
        if (modifiedTime == mostRecentTime)
        {
            std::cout << "Serializing database\n";

            // Convert database to string.
            std::ifstream input(dbName, std::ios::binary);
            std::string dbStr((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

            // Send database to replicas.
            for (auto socket : replicas)
            {
                if (network.sendMessage(socket, {Network::SYNC, dbStr}) < 0)
                {
                    return -1;
                }
            }
        }
        else
        {
            std::cout << "Waiting for database sync\n";

            while (dbSyncedStr.empty())
            {
                // Wait for database to be synced.
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }

            // Convert string to database.
            std::ofstream output(dbName, std::ios::binary | std::ios::trunc);
            output << dbSyncedStr;
        }
    }

    std::cout << "Opening database file\n";

    // Open database.
    int r = sqlite3_open(dbName.c_str(), &db);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        return -1;
    }

    // Enable foreign keys.
    r = sqlite3_exec(db, "PRAGMA foreign_keys = ON", NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        return -1;
    }

    // Create users table if it does not already exist.
    r = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY)", NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        return -1;
    }

    // Create messages table if it does not already exist.
    r = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, sender TEXT, receiver TEXT, message TEXT, timestamp TEXT, FOREIGN KEY(receiver) REFERENCES users(username) ON DELETE CASCADE)", NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        return -1;
    }

    return 0;
}

Network::Message Server::handleTime(Network::Message message)
{
    std::unique_lock lk(dbSync_m);
    dbModifiedTimes.push_back(std::stoull(message.data));
    return {Network::NO_RETURN};
}

Network::Message Server::handleSync(Network::Message message)
{
    dbSyncedStr = message.data;
    db_cv.notify_all();
    return {Network::NO_RETURN};
}

Network::Message Server::createAccount(Network::Message info)
{
    doReplication(info);
    std::unique_lock lock(db_m);

    std::string user = info.data;
    if (user.size() == 0)
    {
        return {Network::ERROR, "No username provided"};
    }

    // Find if user already exists.
    std::string existingUsers = "";
    std::string sql = std::string("SELECT username FROM users WHERE username = '") + user + "'";

    auto callback = [](void *data, int numCols, char **colData, char **colNames) -> int
    {
        std::string username = colData[0];
        std::string *existingUsers = (std::string *)data;
        *existingUsers = username;
        return 0;
    };

    int r = sqlite3_exec(db, sql.c_str(), callback, &existingUsers, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    if (existingUsers.length() > 0)
    {
        return {Network::ERROR, "User already exists"};
    }

    std::cout << "Creating account: " << user << "\n";

    // Insert user into database.
    sql = std::string("INSERT INTO users VALUES ('") + user + "')";
    r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    return {Network::CREATE, user};
}

Network::Message Server::listAccounts(Network::Message requester)
{
    std::unique_lock lock(db_m);
    std::string result;

    // Find all users that match the substring.
    std::string substr = requester.data;
    std::string sql = std::string("SELECT username FROM users WHERE username LIKE '%") + substr + "%'";

    auto callback = [](void *data, int numCols, char **colData, char **colNames) -> int
    {
        std::string username = colData[0];
        std::string *result = (std::string *)data;
        *result += username + "\n";
        return 0;
    };

    int r = sqlite3_exec(db, sql.c_str(), callback, &result, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    std::cout << "Sending account list\n";

    return {Network::LIST, result};
}

Network::Message Server::deleteAccount(Network::Message requester)
{
    doReplication(requester);
    std::unique_lock lock(db_m);

    std::string user = requester.data;

    // Find if user exists.
    std::string existingUsers = "";
    std::string sql = std::string("SELECT username FROM users WHERE username = '") + user + "'";

    auto callback = [](void *data, int numCols, char **colData, char **colNames) -> int
    {
        std::string username = colData[0];
        std::string *existingUsers = (std::string *)data;
        *existingUsers = username;
        return 0;
    };

    int r = sqlite3_exec(db, sql.c_str(), callback, &existingUsers, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    if (existingUsers.length() == 0)
    {
        return {Network::ERROR, "User does not exist"};
    }

    std::cout << "Deleting account: " << user << "\n";

    // Delete user from database.
    sql = std::string("DELETE FROM users WHERE username = '") + user + "'";
    r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    return {Network::DELETE, user};
}

Network::Message Server::sendMessage(Network::Message message)
{
    doReplication(message);
    std::unique_lock lock(db_m);

    // Insert message into database.
    std::string sender = message.sender, receiver = message.receiver;
    std::string messageText = message.data;

    std::cout << "Enqueing message from " << sender << " to " << receiver << "\n";

    std::string sql = std::string("INSERT INTO messages (sender, receiver, message, timestamp) VALUES ('") + sender + "', '" + receiver + "', '" + messageText + "', datetime('now'))";
    int r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    return {Network::OK};
}

Network::Message Server::requestMessages(Network::Message message)
{
    doReplication(message);
    std::unique_lock lock(db_m);

    std::string result;

    std::string user = message.data;
    if (user.size() <= 0 || user[0] == '\0')
    {
        return {Network::SEND, ""};
    }

    // Find all messages for the user.
    std::string sql = std::string("SELECT sender, message, timestamp FROM messages WHERE receiver = '") + user + "' ORDER BY timestamp";

    auto callback = [](void *data, int numCols, char **colData, char **colNames) -> int
    {
        if (numCols != 3)
        {
            perror("Invalid number of columns");
            exit(1);
        }

        std::string sender = colData[0], messageText = colData[1], timestamp = colData[2];
        std::string *result = (std::string *)data;
        *result += std::string("[") + timestamp + "] " + sender + ": " + messageText + "\n";
        return 0;
    };

    int r = sqlite3_exec(db, sql.c_str(), callback, &result, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    // Delete all messages for the user.
    sql = std::string("DELETE FROM messages WHERE receiver = '") + user + "'";
    r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    return {Network::SEND, result};
}

Network::Message Server::handleLeader(Network::Message message)
{
    try
    {
        int id = std::stoi(message.data);
        runningReplicas.insert(id);
        leaderId = id;
    }
    catch (...)
    {
    }

    return {Network::NO_RETURN};
}

Network::Message Server::handleIdentify(Network::Message message)
{
    try
    {
        int id = std::stoi(message.data);
        runningReplicas.insert(id);
    }
    catch (...)
    {
    }

    return {Network::NO_RETURN};
}

void Server::discoverReplicas()
{
    // Send out ID to other replicas. Identify ourselves as the leader if we are.
    Network::Message identifyMessage{Network::IDENTIFY, std::to_string(replicaId)};
    if (isLeader)
    {
        identifyMessage.operation = Network::LEADER;
    }

    for (auto i = replicas.begin(); i != replicas.end();)
    {
        int socket = *i;
        int ret = network.sendMessage(socket, identifyMessage);
        if (ret < 0)
        {
            // Replica is down.
            i = replicas.erase(i);
        }
        else
        {
            ++i;
        }
    }

    // Wait to receive IDs from other replicas.
    while (runningReplicas.size() < replicas.size())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    if (runningReplicas.find(leaderId) == runningReplicas.end() && !isLeader)
    {
        // Leader went down.
        doLeaderElection();
    }
}

void Server::doLeaderElection()
{
    std::cout << "Leader has gone down. Performing leader election." << std::endl;
    runningReplicas.insert(replicaId);

    int minId = *runningReplicas.begin();
    if (minId == replicaId)
    {
        std::cout << "This server is the new leader." << std::endl;
        isLeader = true;
        network.isServer = false;
        for (auto socket : replicas)
        {
            network.sendMessage(socket, {Network::LEADER, std::to_string(replicaId)});
        }
    }

    runningReplicas.clear();
}

void Server::doReplication(Network::Message message)
{
    // If we are not the leader, then there is nothing to do.
    if (!isLeader)
    {
        return;
    }

    // Otherwise, tell the replicas about the new message.
    for (auto i = replicas.begin(); i != replicas.end();)
    {
        int socket = *i;
        int ret = network.sendMessage(socket, message);
        if (ret < 0)
        {
            // Replica is down.
            i = replicas.erase(i);
        }
        else
        {
            ++i;
        }
    }
}

int Server::acceptClient()
{
    int clientSocket;
    struct sockaddr_in address;
    size_t addressLength;

    addressLength = sizeof(address);
    clientSocket = accept(serverFd, (struct sockaddr *)&address,
                          (socklen_t *)&addressLength);
    if (clientSocket < 0)
    {
        perror("accept()");
        return clientSocket;
    }

    // Enable TCP KeepAlive to ensure that we are notified if the leader goes down.
    int enable = 1;
    if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(int)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

    struct linger lo = {1, 0};
    if (setsockopt(clientSocket, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo)) < 0)
    {
        perror("setsockopt()");
        return -1;
    }

    std::thread socketThread(&Server::processClient, this, clientSocket);
    socketThread.detach();

    return 0;
}

int Server::processClient(int socket)
{
    while (serverRunning)
    {
        int err = network.receiveOperation(socket);
        if (err < 0)
        {
            discoverReplicas();
            break;
        }
    }
    close(socket);

    return 0;
}

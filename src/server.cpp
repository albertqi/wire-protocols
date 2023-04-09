#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sqlite3.h>

#include <netinet/in.h>
#include <sys/socket.h>

#include "server.hpp"

Server::Server(int port)
{
    // Initialize socket
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0)
    {
        perror("socket()");
        exit(1);
    }
    // Enable SO_REUSEADDR so that we dont get bind() errors if the previous
    // socket is stuck in TIME_WAIT or hasn't been released by the OS.
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

    // Register callbacks.
    network.registerCallback(Network::CREATE, Callback(this, &Server::createAccount));
    network.registerCallback(Network::DELETE, Callback(this, &Server::deleteAccount));
    network.registerCallback(Network::SEND, Callback(this, &Server::sendMessage));
    network.registerCallback(Network::LIST, Callback(this, &Server::listAccounts));
    network.registerCallback(Network::REQUEST, Callback(this, &Server::requestMessages));

    // Open database.
    int r = sqlite3_open("server.db", &db);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    // Create users table if it does not already exist.
    r = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS users (username TEXT PRIMARY KEY)", NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    // Create messages table if it does not already exist.
    r = sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS messages (id INTEGER PRIMARY KEY, sender TEXT, receiver TEXT, message TEXT, timestamp TEXT, FOREIGN KEY(receiver) REFERENCES users(username))", NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    serverRunning = true;
}

void Server::stopServer()
{
    sqlite3_close(db);
    serverRunning = false;
}

Network::Message Server::createAccount(Network::Message info)
{
    std::string user = info.data;
    if (user.size() == 0)
    {
        return {Network::ERROR, "No username provided"};
    }

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
    std::string result;

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
    std::string user = requester.data;

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

    sql = std::string("DELETE FROM users WHERE username = '") + user + "'";
    r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    // TODO: DROP USER'S MESSAGES IN QUEUE

    return {Network::DELETE, user};
}

Network::Message Server::sendMessage(Network::Message message)
{
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
    std::string result;

    std::string user = message.data;
    if (user.size() <= 0 || user[0] == '\0')
    {
        return {Network::SEND, ""};
    }

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

    std::cout << "Delivering messages to " << user << "\n";

    int r = sqlite3_exec(db, sql.c_str(), callback, &result, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    sql = std::string("DELETE FROM messages WHERE receiver = '") + user + "'";
    r = sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
    if (r != SQLITE_OK)
    {
        perror(sqlite3_errmsg(db));
        exit(1);
    }

    return {Network::SEND, result};
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
            break;
        }
    }

    close(socket);

    return 0;
}

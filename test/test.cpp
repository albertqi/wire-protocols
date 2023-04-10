#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <regex>
#include <chrono>

#define PADDED_LENGTH 50

using namespace std::literals::chrono_literals;

bool allTestsPassed = true;
std::mutex m;

void test(bool b, std::string msg)
{
    int i = msg.size();
    msg += b ? "PASSED" : "FAILED";
    msg.insert(i, PADDED_LENGTH - msg.size(), ' ');
    allTestsPassed = allTestsPassed && b;
    std::cerr << msg << std::endl;
}

bool operator==(const Network::Message a, const Network::Message b)
{
    return (a.operation == b.operation) && (a.data == b.data) &&
           (a.sender == b.sender) && (a.receiver == b.receiver);
}

void testServer(Server &server, Client &client)
{
    // Test `createAccount`
    test(server.createAccount({Network::CREATE, ""}) ==
         (Network::Message){Network::ERROR, "No username provided", "", ""},
         "createAccount empty");
    test(server.createAccount({Network::CREATE, "abc"}) ==
         (Network::Message){Network::CREATE, "abc", "", ""},
         "createAccount simple");
    test(server.createAccount({Network::CREATE, "abcdef"}) ==
         (Network::Message){Network::CREATE, "abcdef", "", ""},
         "createAccount medium");
    test(server.createAccount({Network::CREATE, "123abcdef456"}) ==
         (Network::Message){Network::CREATE, "123abcdef456", "", ""},
         "createAccount long");
    test(server.createAccount({Network::CREATE, "abc"}) ==
         (Network::Message){Network::ERROR, "User already exists", "", ""},
         "createAccount duplicate");

    // Test `deleteAccount`
    test(server.deleteAccount({Network::DELETE, ""}) ==
         (Network::Message){Network::ERROR, "User does not exist", "", ""},
         "deleteAccount empty");
    test(server.deleteAccount({Network::DELETE, "abc"}) ==
         (Network::Message){Network::DELETE, "abc", "", ""},
         "deleteAccount simple");
    test(server.deleteAccount({Network::DELETE, "abc"}) ==
         (Network::Message){Network::ERROR, "User does not exist", "", ""},
         "deleteAccount no user");

    // Test `listAccounts`
    test(server.listAccounts({Network::LIST, "123"}) ==
         (Network::Message){Network::LIST, "123abcdef456\n", "", ""},
         "listAccounts substring one");
    test(server.listAccounts({Network::LIST, "abcdef"}) ==
         (Network::Message){Network::LIST, "abcdef\n123abcdef456\n", "", ""},
         "listAccounts substring both");
    test(server.listAccounts({Network::LIST, "abc123"}) ==
         (Network::Message){Network::LIST, "", "", ""},
         "listAccounts substring none");
    test(server.listAccounts({Network::LIST, ""}) ==
         (Network::Message){Network::LIST, "abcdef\n123abcdef456\n", "", ""},
         "listAccounts all");

    // Test `sendMessage`
    test(server.sendMessage({Network::SEND,
         "hello", "abcdef", "123abcdef456"}) ==
         (Network::Message){Network::OK, "", "", ""},
         "sendMessage simple");
    test(server.sendMessage({Network::SEND,
         "the quick brown fox jumps over the lazy dog", "abcdef", "123abcdef456"}) ==
         (Network::Message){Network::OK, "", "", ""},
         "sendMessage long");

    // Test `requestMessages`
    test(server.requestMessages({Network::REQUEST, ""}) ==
         (Network::Message){Network::SEND, "", "", ""},
         "requestMessages no user");
    test(server.requestMessages({Network::REQUEST, "abcdef"}) ==
         (Network::Message){Network::SEND, "", "", ""},
         "requestMessages empty");
    test(std::regex_match(server.requestMessages({Network::REQUEST, "123abcdef456"}).data, 
         std::regex("\\[.*\\] abcdef: hello\n\\[.*\\] abcdef: the quick brown fox jumps over the lazy dog\n")),
         "requestMessages multiple");
    test(server.requestMessages({Network::REQUEST, "123abcdef456"}) ==
         (Network::Message){Network::SEND, "", "", ""},
         "requestMessages all read");
}

void testClient(Server &server, Client &client)
{
    // Test `clientRunning`
    test(client.clientRunning, "clientRunning");

    // Test `createAccount`
    test(client.createAccount("") == "No username provided",
         "createAccount empty");
    test(client.createAccount("user") == "Created account user",
         "createAccount simple");
    test(client.createAccount("user123") == "Created account user123",
         "createAccount medium");
    test(client.createAccount("123user456user789") ==
         "Created account 123user456user789",
         "createAccount long");
    test(client.createAccount("user") == "User already exists",
         "createAccount duplicate");

    // Test `deleteAccount`
    test(client.deleteAccount("") == "User does not exist",
         "deleteAccount empty");
    test(client.deleteAccount("123user456user789") ==
         "Deleted account 123user456user789",
         "deleteAccount simple");
    test(client.deleteAccount("123user456user789") == "User does not exist",
         "deleteAccount no user");

    // Test `getAccountList`
    test(client.getAccountList("user123") == "user123\n",
         "getAccountList substring one");
    test(client.getAccountList("user") == "user\nuser123\n",
         "getAccountList substring both");
    test(client.getAccountList("123user") == "",
         "getAccountList substring none");
    test(client.getAccountList("") == "abcdef\n123abcdef456\nuser\nuser123\n",
         "getAccountList all");

    // Test `getClientUserList`
    test(client.getClientUserList().size() == 4,
         "getClientUserList size");
    test(client.getClientUserList().find("user") !=
         client.getClientUserList().end(),
         "getClientUserList contains user");
    test(client.getClientUserList().find("123user") ==
         client.getClientUserList().end(),
         "getClientUserList not contains user");

    // Test `getCurrentUser` and `setCurrentUser`
    test(client.getCurrentUser() == "",
         "getCurrentUser no user");
    client.setCurrentUser("user123");
    test(client.getCurrentUser() == "user123",
         "getCurrentUser simple");

    // Test `handleCreateResponse`
    test(client.handleCreateResponse({Network::CREATE, "testing", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleCreateResponse simple");
    test(client.handleCreateResponse({Network::CREATE,
         "testingtesting", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleCreateResponse medium");
    test(client.handleCreateResponse({Network::CREATE,
         "testingtestingtesting", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleCreateResponse long");

    // Test `handleDelete`
    test(client.handleDelete({Network::DELETE, "testing", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleDelete simple");
    test(client.handleDelete({Network::DELETE, "testingtesting", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleDelete medium");
    test(client.handleDelete({Network::DELETE,
         "testingtestingtesting", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleDelete long");

    // Test `handleList`
    test(client.handleList({Network::LIST, "testing\n", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleList simple");
    test(client.handleList({Network::LIST, "testing\ntesting123\n", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleList medium");
    test(client.handleList({Network::LIST,
         "testing\ntesting123\n123testing123", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleList long");

    // Test `handleReceive`
    test(client.handleReceive({Network::SEND, "testing: hello\n", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleReceive simple");
    test(client.handleReceive({Network::SEND,
         "testing123: goodbye\n", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleReceive medium");
    test(client.handleReceive({Network::SEND,
         "testing: hello\ntesting123: goodbye\n", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "handleReceive long");

    // Test `messageCallback`
    test(client.messageCallback({Network::OK, "", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "messageCallback ok");
    test(client.messageCallback({Network::ERROR,
         "User does not exist", "", ""}) ==
         (Network::Message){Network::NO_RETURN, "", "", ""},
         "messageCallback error");

    // Test `sendMessage`
    test(client.sendMessage({Network::SEND, "hello", "user123", "user"}) == "",
         "sendMessage simple");
    test(client.sendMessage({Network::SEND,
         "the quick brown fox jumps over the lazy dog", "user123", "user"}) == "",
         "sendMessage long");
}

int main()
{
    // Remove existing databases
    std::remove("server_3000.db");
    std::remove("server_3001.db");
    std::remove("server_3002.db");

    // Create list of servers
    std::vector<std::pair<std::string, int>> serverList{{"127.0.0.1", 3000}, {"127.0.0.1", 3001}, {"127.0.0.1", 3002}};

    // Create list of replicas for each server
    std::vector<std::pair<std::string, int>> replicas0{{"127.0.0.1", 3001}, {"127.0.0.1", 3002}};
    std::vector<std::pair<std::string, int>> replicas1{{"127.0.0.1", 3000}, {"127.0.0.1", 3002}};
    std::vector<std::pair<std::string, int>> replicas2{{"127.0.0.1", 3000}, {"127.0.0.1", 3001}};

    // Create pointers to server objects
    Server *server0, *server1, *server2;

    // Run initial tests
    std::cerr << "\nRUNNING INITIAL TESTS..." << std::endl;
    std::thread t0([&replicas0, &server0]()
    {
        server0 = new Server(3000, 0, replicas0);
        m.lock();
        for (int i = 0; i < 2; i++)
          test((*server0).acceptClient() == 0, "initalServerConnect server0");
        m.unlock();
    });
    std::thread t1([&replicas1, &server1]()
    {
        server1 = new Server(3001, 1, replicas1);
        m.lock();
        for (int i = 0; i < 2; i++)
          test((*server1).acceptClient() == 0, "initalServerConnect server1");
        m.unlock();
    });
    std::thread t2([&replicas2, &server2]()
    {
        server2 = new Server(3002, 2, replicas2);
        m.lock();
        for (int i = 0; i < 2; i++)
          test((*server2).acceptClient() == 0, "initalServerConnect server2");
        m.unlock();
    });

    t0.join();
    t1.join();
    t2.join();

    // Setup databases for each server.
    std::thread syncT0([&server0]()
    {
        test(server0->syncDatabases(3000) == 0, "syncDatabases server0");
    });
    std::thread syncT1([&server1]()
    {
        test(server1->syncDatabases(3001) == 0, "syncDatabases server1");
    });
    std::thread syncT2([&server2]()
    {
        test(server2->syncDatabases(3002) == 0, "syncDatabases server2");
    });
    std::this_thread::sleep_for(100ms);

    // Join the threads
    
    syncT0.join();
    syncT1.join();
    syncT2.join();

     std::thread t4([&replicas0, &server0]()
     {
          m.lock();
          test((*server0).acceptClient() == 0, "acceptClient server0");
          m.unlock();
     });

     // Create client object
    Client client(serverList);

    t4.join();

    // Run server tests
    std::cerr << "\nRUNNING SERVER TESTS..." << std::endl;
    testServer(*server0, client);

    // Run client tests
    std::cerr << "\nRUNNING CLIENT TESTS..." << std::endl;
    testClient(*server0, client);

    // Clean up client
    client.stopClient();

    // Run final tests
    std::cerr << "\nRUNNING FINAL TESTS..." << std::endl;
    test(!client.clientRunning, "clientRunning stopped");

    // Clean up servers
    (*server0).stopServer();
    (*server1).stopServer();
    (*server2).stopServer();

    delete server0;
    delete server1;
    delete server2;

    // Print results
    std::string msg = allTestsPassed ? "\nALL TESTS PASSED :)\n" :
                                       "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;

    return 0;
}

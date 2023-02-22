#include "server.hpp"
#include "client.hpp"
#include <iostream>
#include <string>
#include <thread>

#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>

#define PADDED_LENGTH 50

bool allTestsPassed = true;

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

#ifndef ENABLE_GRPC
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
         (Network::Message){Network::LIST, "123abcdef456\nabcdef\n", "", ""},
         "listAccounts substring both");
    test(server.listAccounts({Network::LIST, "abc123"}) ==
         (Network::Message){Network::LIST, "", "", ""},
         "listAccounts substring none");
    test(server.listAccounts({Network::LIST, ""}) ==
         (Network::Message){Network::LIST, "123abcdef456\nabcdef\n", "", ""},
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
    test(server.requestMessages({Network::REQUEST, "123abcdef456"}) ==
         (Network::Message){Network::SEND,
         "abcdef: hello\nabcdef: the quick brown fox jumps over the lazy dog\n", "", ""},
         "requestMessages multiple");
    test(server.requestMessages({Network::REQUEST, "123abcdef456"}) ==
         (Network::Message){Network::SEND, "", "", ""},
         "requestMessages all read");
}
#endif

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
    test(client.getAccountList("user") == "user123\nuser\n",
         "getAccountList substring both");
    test(client.getAccountList("123user") == "",
         "getAccountList substring none");
    test(client.getAccountList("") == "user123\nuser\n123abcdef456\nabcdef\n",
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

#ifndef ENABLE_GRPC
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
#endif

    // Test `requestMessages`
    test(client.requestMessages() ==
         "testing: hello\ntesting123: goodbye\n",
         "requestMessages simple");

    // Test `sendMessage`
    test(client.sendMessage({Network::SEND, "hello", "user123", "user"}) == "",
         "sendMessage simple");
    test(client.sendMessage({Network::SEND,
         "the quick brown fox jumps over the lazy dog", "user123", "user"}) == "",
         "sendMessage long");
}

int main()
{    
#ifdef ENABLE_GRPC
    Server service;

    std::string server_address = "0.0.0.0:1111";
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
#else
    Server server(1111);
#endif

#ifndef ENABLE_GRPC
    std::cerr << "\nRUNNING INITIAL TESTS..." << std::endl;
    std::thread t([&server]()
    {
        test(server.acceptClient() == 0, "acceptClient");
    });
#endif


#ifdef ENABLE_GRPC
     Client client(grpc::CreateChannel("localhost:1111",
                          grpc::InsecureChannelCredentials()));
#else
     Client client("127.0.0.1", 1111);
    t.join();
#endif

#ifndef ENABLE_GRPC
    std::cerr << "\nRUNNING SERVER TESTS..." << std::endl;
    testServer(server, client);
#endif

    std::cerr << "\nRUNNING CLIENT TESTS..." << std::endl;
    testClient(server, client);

    client.stopClient();

    std::cerr << "\nRUNNING FINAL TESTS..." << std::endl;
    test(!client.clientRunning, "clientRunning stopped");

#ifndef ENABLE_GRPC
    server.stopServer();
#endif

    std::string msg = allTestsPassed ? "\nALL TESTS PASSED :)\n" :
                                       "\nSOME TESTS FAILED :(\n";
    std::cerr << msg << std::endl;
}

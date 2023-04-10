/**
 * The `Network` class is the data link layer abstraction for both the server and
 * client and defines the common wire protocol the server and client use to
 * communicate. The following breaks down how a user interacts with this class:
 *
 * 1. Register callbacks with `Network` for each operation (`OpCode`) the user
 *    wants to handle.
 * 2. The user calls the `receiveOperation()` function for a particular
 *    connection. When the `Network` instance receives an operation on the
 *    connection, the callback for that operation is triggered.
 * 3. The results of that operation can returned by the user using
 *    `sendMessage()`.
 *
 * Communication over the network and the parsing of operations, metadata, and
 * data are handled by this class as well. Notably, this class does not initate
 * connections or handle the closing (unexepected or intentional) of connections.
 * The user of this class must handle possible `SIGPIPE`s and manage the socket
 * file descriptor.
 *
 * Every packet sent between `Network` instances must follow the following wire
 * protocol:
 *
 * //////// Header ////////
 * > Protocol version number (4 bytes)
 * > Operation (4 bytes)
 * > Sender information data length (8 bytes)
 * > Receiver information data length (8 bytes)
 * > Data length (8 bytes)
 * ///////// Data /////////
 * > Sender information of length `senderLength` (Could be 0)
 * > Reciever information of length `recieverLength` (Could be 0)
 * > Operation data of length `dataLength`
 *
 * Any data received by this class that does not follow the above protocol will
 * be ignored.
 *
 * ///////////////////////////// Usage Information /////////////////////////////
 *
 * Function information and return values:
 * All functions in `Network` return an int which is the return value of network
 * operation used in the function. If a function has an additional return value,
 * it is returned in an output parameter at the end of the argument list, marked
 * by <argumentName>Out.
 *
 * Network return values and error codes:
 * Errors from the server to the client are returned using the `ERROR` op code.
 * The client is not allowed to send errors to the server. Normal return values
 * from the server will be sent to the client using the appropriate operation.
 * For messages, the client will receive a `SEND` operation. For a list of users,
 * the client will receive a `LIST`. For any other void function such as sending
 * a message to another user, the client will receive `OK` on success, or `ERROR`
 * with a message on failure.
 *
 * Callbacks and Receiving Data from the Client/Server:
 * When the `receiveOperation()` function is called by either the client or
 * server, the function will block until an `OpCode` is received on the
 * designated socket. Any subsequent data received will be parsed by this class
 * and passed to the registered callback for that operation in a `Message`
 * object. The fields of the `Message` object are only defined for some
 * oeprations. For example, in a `LIST` op, only the `data` field is defined and
 * `sender` and `receiver` are empty. Some operations have no fields defined.
 */

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>

#define VERSION 1

// Forward declare Client and Server so the Network class can register callbacks.
class Server;
class Client;
class Callback;

class Network
{
public:

    /**
     * Defines the possible operations defined by this wire protocol. Each
     * operation is annotated with what part of `Message` is defined when
     * `receiveOperation` is called.
    */
    enum OpCode : uint32_t
    {
        // Server -> Client operations.
        OK, // Contains data.
        ERROR, // Contains data.

        // Client -> Server operations.
        // Contains data.
        CREATE,  // Contains data
        DELETE,  // Contains data
        REQUEST, // Contains data

        // Bi-directional operations.
        SEND, // Contains data, sender, receiver
        LIST,

        // Server -> Server operations.
        LEADER, // Contains data
        IDENTIFY, // Contains data
        SYNC, // Contains data
        TIME, // Contains data

        // Other
        UNSUPPORTED_OP,
        NO_RETURN
    };

    const std::unordered_set<OpCode> serverOperations = {
        LEADER, IDENTIFY, SYNC, TIME
    };

    /**
     * Generic Message object that is passed as context to each callback. Not
     * every field of this object is defined for every operation. For some
     * operations, no fields are defined. A default generated `Message` object
     * is valid and is a simple `OK` operation.
     */
    struct Message
    {
        OpCode operation;
        std::string data;
        std::string sender;
        std::string receiver;
    };

    /**
     * Waits for an operation to be received from `socket`. Triggers the
     * registered callback for that operation with the appropriate data recieved
     * from from the connection.
     *
     * @return  Socket read() errors.
     *          Errors returned by the callback.
     */
    int receiveOperation(int socket);

    /**
     * Send the given `Message` object to the peer on `socket` following the
     * wire protocol defined by this class.
     *
     * @return  Socket send() errors.
     */
    int sendMessage(int socket, Message message);

    /**
     * Convenience function to send an error message to the peer.
     *
     * @return  Socket send() errors.
     */
    int sendError(int socket, std::string errorMsg);

    /**
     * Save the given function `callback` to be triggered when `operation` is
     * received by this instance.
     */
    void registerCallback(OpCode operation, Callback function);

    /**
     * Whether or not to drop responses originating from a server.
    */
    bool dropServerResponses = false;

    /**
     * Whether or not to mark messages as originating from a server.
    */
    bool isServer = false;

private:

    /**
     * Header for any data sent between the server and client.
     */
    struct Metadata
    {
        // Protocol version number.
        uint32_t version;
        // The requested/received operation.
        OpCode operation;
        // Length of the username of the sender. 0 if unused.
        uint64_t senderLength;
        // Length of the username of the receiver. 0 if unused.
        uint64_t receiverLength;
        // Length of the data following this metadata header
        // (not including the sender/recvier information).
        uint64_t dataLength;
        // If this message originate from a server.
        bool isServer;
    };

    /**
     * Mappings from operations to user callbacks.
     */
    std::unordered_map<OpCode, Callback> registered_callbacks;
};

/**
 * Common wrapper class for `Server` and `Client` member functions.
*/
class Callback
{
public:

    Callback(Server* instance, Network::Message (Server::*func)(Network::Message));

    Callback(Client* instance, Network::Message (Client::*func)(Network::Message));

    Network::Message operator()(Network::Message message);

private:

    bool isClientCallback;
    Network::Message (Server::*serverCallback)(Network::Message);
    Network::Message (Client::*clientCallback)(Network::Message);

    Client* client;
    Server* server;
};

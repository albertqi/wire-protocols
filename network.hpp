/**
 * The `Network` class is the network layer abstraction for both the server and
 * client. The class also serves as the data link layer and defines the common
 * wire protocol the server and client use to communicate. The following breaks
 * down how a user interacts with this class:
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
 * data are handled by this class as well. Every packet sent between `Network`
 * instances must follow the following wire protocol:
 * 
 * > Operation (VERSION, OpCode)
 * > Metadata  (dataLength, senderLength, receiverLength)
 * > Sender information of length `senderLength` (Could be 0)
 * > Reciever information of length `recieverLength` (Could be 0)
 * > Operation data of length `dataLength`
 * 
 * Any data received by this class that does not follow the above protocol will
 * be ignored.
 * 
 * Function information and return values:
 * All functions in `Network` return an int which is the return value of network
 * operation used in the function. The semantic return value of the function
 * is returned in an output parameter at the end of the argument list, marked by
 * <argumentName>Out.
 * 
 * Network return values and error codes:
 * Errors from the server to the client are returned using the `ERROR` op code.
 * The client is not allowed to send errors to the server. Normal return values
 * from the server will be sent to the client using the appropriate operation.
 * For messages, the client will receive a `SEND` operation. For a list of users,
 * the client will receive a `LIST`. For any other void function such as sending
 * a message to another user, the client will receive `OK` on success, or `ERROR`
 * with a message on failure.
 */

#include <string>
#include <unordered_map>

#define VERSION 1

class Network
{
public:
    enum OpCode
    {
        // Client -> Server operations.
        CREATE,
        DELETE,

        // Server -> Client operations.
        ERROR,
        OK,

        // Bi-directional operations.
        SEND,
        LIST,
    };

    struct Operation
    {
        int version = VERSION;
        OpCode code;
    };
    
    /**
     * Header for any data sent between the server and client.
     */
    struct Metadata
    {
        // Length of the data following this metadata header.
        size_t dataLength;
        // Length of the username of the sender. 0 if unused.
        size_t senderLength;
        // Length of the username of the receiver. 0 if unused.
        size_t receiverLength;
    };

    /**
     * Generic Message object that is passed as context to each callback. Not
     * every field of this object is defined for every operation. For some
     * operations, no fields are defined.
    */
    struct Message
    {
        std::string data;
        std::string sender;
        std::string receiver;
    };

    typedef int (*callback)(Message);

    /**
     * Waits for an operation to be received from `socket`. Triggers the
     * registered callback for that operation with the appropriate data recieved
     * from from the connection.
     * 
     * Returns:
     *      Socket read() errors.
     *      Errors returned by the callback.
     */
    int receiveOperation(int socket);

    int sendMessage(int socket, Metadata metadata, std::string data);

    void register_callback(OpCode operation, callback function);

private:
    std::unordered_map<OpCode, callback> registered_callbacks;

    int receiveMetadata(int socket, Metadata &metadataOut);

    int receiveData(int socket, size_t dataLength, std::string &dataOut);
};

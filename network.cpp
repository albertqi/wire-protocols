#include <sys/socket.h>
#include <unistd.h>

#include "network.hpp"

// MAC and Linux have different names for telling the OS you have more data to
// send.
#ifdef __linux__
    #define MSG_HAVEMORE MSG_MORE
#endif

int Network::receiveOperation(int socket)
{
    int err;
    Metadata header;

    // Read header from the socet.
    err = read(socket, &header, sizeof(Metadata));
    if (err < 0)
    {
        return err;
    }
    // Version checking works to both ensure that the network protocols are in
    // agreement as well make sure that the wire protocol is being followed at
    // all.
    if (header.version != VERSION)
    {
        // TODO: send error to message
        return -1;
    }

    // Read sender information if available.
    std::string sender;
    sender.reserve(header.senderLength);
    err = read(socket, sender.data(), header.senderLength);
    if (err < 0)
    {
        return err;
    }

    // Read receiver information if available.
    std::string receiver;
    receiver.reserve(header.receiverLength);
    err = read(socket, receiver.data(), header.receiverLength);
    if (err < 0)
    {
        return err;
    }

    // Read operation data.
    std::string data;
    data.reserve(header.dataLength);
    err = read(socket, data.data(), header.dataLength);
    if (err < 0)
    {
        return err;
    }

    // Construct Message object
    Message message = {
        header.operation,
        data,
        sender,
        receiver
    };

    // Check that a callback has been registered for the received operation.
    if (registered_callbacks.find(header.operation) != registered_callbacks.end())
    {
        callback func = registered_callbacks[header.operation];
        err = func(message);
    }

    return err;
}

int Network::sendMessage(int socket, Message message)
{
    // Setup protocol header.
    Metadata header = {
        VERSION,
        message.operation,
        message.sender.size(),
        message.receiver.size(),
        message.data.size()
    };

    int err;
    
    // Send header.
    err = send(socket, &header, sizeof(Metadata), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }
    // Send sender information.
    char* senderData = message.sender.data();
    err = send(socket, senderData, message.sender.size(), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }
    // Send receiver information.
    char* receiverData = message.receiver.data();
    err = send(socket, receiverData, message.receiver.size(), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }
    // Send the rest of the data.
    char* data = message.data.data();
    err = send(socket, data, message.data.size(), 0);

    return err;
}

int Network::sendError(int socket, std::string errorMsg)
{
    Message message = {
        ERROR,
        errorMsg
    };

    return sendMessage(socket, message);
}

void Network::register_callback(OpCode operation, callback function)
{
    registered_callbacks[operation] = function;
}

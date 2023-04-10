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
    Metadata header{};

    // Read header from the socket.
    err = read(socket, &header, sizeof(Metadata));
    if (err <= 0)
    {
        return (err == 0) ? -1 : err;
    }

    // Version checking works to both ensure that the network protocols are in
    // agreement as well as make sure that the wire protocol is being followed
    // at all.
    if (header.version != VERSION)
    {
        sendError(socket, "Incompatible protocol version.");
        return -1;
    }

    // Read sender information if available.
    std::string sender(header.senderLength, 0);
    err = read(socket, &sender[0], header.senderLength);
    if (err < 0)
    {
        return err;
    }

    // Read receiver information if available.
    std::string receiver(header.receiverLength, 0);
    err = read(socket, &receiver[0], header.receiverLength);
    if (err < 0)
    {
        return err;
    }

    // Read operation data.
    std::string data(header.dataLength, 0);
    size_t dataIndex = 0;
    size_t bytesToRead = header.dataLength;
    while (bytesToRead > 0)
    {
        err = read(socket, &data[dataIndex], bytesToRead);
        if (err < 0)
        {
            return err;
        }
        bytesToRead -= err;
        dataIndex += err;
    }

    // Construct Message object.
    Message message = {
        header.operation,
        data,
        sender,
        receiver,
    };
    Message output;

    // Message originated from server, and we do not care about those.
    if (header.isServer && dropServerResponses &&
        serverOperations.find(message.operation) == serverOperations.end())
    {
        return err;
    }

    // Check that a callback has been registered for the received operation.
    if (registered_callbacks.find(header.operation) != registered_callbacks.end())
    {
        Callback func = registered_callbacks.at(header.operation);
        output = func(message);
        if (output.operation != NO_RETURN)
        {
            err = sendMessage(socket, output);
        }
    }
    // Otherwise, return an unsupported operation message.
    else
    {
        Message unsupportedOp = {UNSUPPORTED_OP};
        err = sendMessage(socket, unsupportedOp);
    }

    return err;
}

int Network::sendMessage(int socket, Message message)
{
    // Set up protocol header.
    Metadata header = {
        VERSION,
        message.operation,
        message.sender.size(),
        message.receiver.size(),
        message.data.size(),
        isServer,
    };

    int err;

    // Send header.
    err = send(socket, &header, sizeof(Metadata), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }

    // Send sender information.
    char *senderData = message.sender.data();
    err = send(socket, senderData, message.sender.size(), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }

    // Send receiver information.
    char *receiverData = message.receiver.data();
    err = send(socket, receiverData, message.receiver.size(), MSG_HAVEMORE);
    if (err < 0)
    {
        return err;
    }

    // Send the rest of the data.
    char *data = message.data.data();
    int bytesSent = 0;
    while (bytesSent < message.data.size())
    {
        err = send(socket, &data[bytesSent], message.data.size() - bytesSent, 0);
        if (err < 0)
        {
            return err;
        }
        bytesSent += err;
    }

    return bytesSent;
}

int Network::sendError(int socket, std::string errorMsg)
{
    Message message = {
        ERROR,
        errorMsg,
    };

    return sendMessage(socket, message);
}

void Network::registerCallback(OpCode operation, Callback function)
{
    registered_callbacks.insert(std::make_pair(operation, function));
}

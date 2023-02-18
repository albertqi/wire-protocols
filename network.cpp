#include <sys/socket.h>
#include <unistd.h>

#include "network.hpp"

// MAC OS and Linux have different names for wanting
#ifdef __linux__
    #define MSG_HAVEMORE MSG_MORE
#endif

int Network::receiveOperation(int socket)
{
    int r;
    Operation op;
    r = read(socket, &op, sizeof(op));
    if (r < 0)
    {
        return r;
    }
    if (op.version != VERSION)
    {
        // TODO: send error to message
        return -1;
    }
    Metadata metadata;
    std::string data;
    switch (op.code)
    {
    case CREATE:
    case SEND:
        r = receiveMetadata(socket, metadata);
        if (r < 0)
        {
            // TODO: send err to client
            return r;
        }
        break;
    }
    callback func = registered_callbacks[op.code];
    r = func(metadata, data);
    return r;
}

int Network::sendMessage(int socket, Metadata metadata, std::string data)
{
    int r;
    Operation op = {.code = SEND};
    r = send(socket, &op, sizeof(op), MSG_HAVEMORE);
    if (r < 0)
    {
        return r;
    }
    r = send(socket, &metadata, sizeof(metadata), MSG_HAVEMORE);
    if (r < 0)
    {
        return r;
    }
    r = send(socket, &data, sizeof(data), MSG_NOSIGNAL);
    return r;
}

void Network::register_callback(OpCode operation, callback function)
{
    registered_callbacks[operation] = function;
}

int Network::receiveMetadata(int socket, Metadata &metadataOut)
{
    Metadata recvd_metadata;
    int r = read(socket, &recvd_metadata, sizeof(recvd_metadata));
    
    return r;
}

int Network::receiveData(int socket, size_t dataLength, std::string &dataOut)
{
    std::string recvd_data;
    int r = read(socket, &recvd_data, dataLength);
    dataOut = recvd_data;

    return r;
}

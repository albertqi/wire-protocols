#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include <unordered_map>
#define VERSION 1

class Network
{
    enum OpCode
    {
        CREATE,
        SEND,
        DELETE,
        LIST,
    };

    struct Operation
    {
        int version = VERSION;
        OpCode code;
    };

    struct Metadata
    {
        int length;
        int sender;
        int receiver;
    };

    typedef int (*callback)(Metadata, std::string);

    std::unordered_map<OpCode, callback> registered_callbacks;

    int receiveOperation(int socket)
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
            r = parse(socket, metadata, data);
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

    int sendMessage(int socket, Metadata metadata, std::string data)
    {
        int r;
        Operation op = {.code = SEND};
        // TODO: look into MSG_HAVEMORE
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

    int parse(int socket, Metadata &metadata, std::string &data)
    {
        Metadata recvd_metadata;
        int r = read(socket, &recvd_metadata, sizeof(recvd_metadata));
        if (r < 0)
        {
            return r;
        }
        std::string recvd_data;
        r = read(socket, &recvd_data, recvd_metadata.length);
        metadata = recvd_metadata;
        data = recvd_data;
        return r;
    }
};
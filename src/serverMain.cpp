#include "server.hpp"
#include <iostream>
#include <string>

/**
 * Starts the server and accepts clients.
 */
int main(int argc, char const *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./server [PORT]" << std::endl;
        return -1;
    }

    int port = std::stoi(argv[1]);

    Server server(port);

    while (true)
    {
        int err = server.acceptClient();
        if (err < 0)
        {
            std::cerr << "Failed client connection" << std::endl;
        }
    }

    return 0;
}
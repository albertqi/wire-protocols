#include <iostream>
#include <filesystem>
#include <fstream>

#include "server.hpp"

/**
 * Starts the server and accepts clients.
*/
int main(int argc, char const *argv[])
{
    std::string server_config_path;
    int replica_number;

    if (argc < 3)
    {
        std::cerr << "Usage: ./server <server.cfg> <server_num>" << std::endl;
        exit(1);
    }

     server_config_path = argv[1];

    if (!std::filesystem::exists(server_config_path))
    {
        std::cerr << "Server config file doesn't exist." << std::endl;
        exit(1);
    }

    try
    {
        replica_number = std::stoi(argv[2]);
    }
    catch (...)
    {
        std::cerr << "Server number must be a number." << std::endl;
        exit(1);
    }


    std::ifstream server_cfg(server_config_path);

    for (int i = 0; )

    Server server(1111);

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

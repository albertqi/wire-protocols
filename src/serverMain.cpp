#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

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

    std::vector<std::pair<std::string, int>> replicas;

    int this_port;

    // Parse server config file. 
    std::string current_line;
    for (int i = 0; std::getline(server_cfg, current_line); i++)
    {
        int colon_index = current_line.find(":");
        std::string server_ip = current_line.substr(0, colon_index);
        int server_port = std::stoi(current_line.substr(colon_index + 1, std::string::npos));

        // If this instance is the current server listing, grab the port to bind to.
        if (i == replica_number)
        {
            this_port = server_port;
        }
        // Put the server in 
        else
        {
            replicas.push_back(std::make_pair(server_ip, server_port));
        }
    }

    Server server(this_port, replica_number, replicas);

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

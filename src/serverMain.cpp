#include "server.hpp"
#include <iostream>

int main(int argc, char const *argv[])
{
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
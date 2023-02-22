#include "server.hpp"
#include "client.hpp"
#include <iostream>

int main()
{
    Server server(1111);
    Client client("127.0.0.1", 1111);

    int r = server.acceptClient();
    if (r != 0)
    {
        std::cerr << "client accepted FAILED" << std::endl;
    }
    std::cerr << "client accepted PASSED" << std::endl;
}

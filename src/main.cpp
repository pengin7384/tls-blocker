#include "blocker.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 5) {
        std::cout << "Usage: ./blocker <target_file> <input_interface> <client-interface> <server_interface>" << std::endl;
        return 1;
    }

    Blocker blocker(argv[1], argv[2], argv[3], argv[4]);
    blocker.start();

    return 0;
}

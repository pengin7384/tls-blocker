#include "blocker.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 4) {
        std::cout << "Usage: ./blocker <target_file> <input_interface> <output_interface>" << std::endl;
        return 1;
    }

    Blocker blocker(argv[1], argv[2], argv[3]);
    blocker.start();

    return 0;
}

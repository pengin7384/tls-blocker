#include "blocker.h"
#include <iostream>

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::cout << "Usage: ./blocker <input_interface> <output_interface>" << std::endl;
        return 1;
    }

    Blocker blocker(argv[1], argv[2]);
    blocker.start();

    return 0;
}

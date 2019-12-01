#pragma once
#include <string>
#include "network_manager.h"

class Blocker {
    NetworkManager net_mng;

public:
    Blocker(std::string in, std::string out)
        : net_mng(in, out) {


    }

    void start() {

    }

};

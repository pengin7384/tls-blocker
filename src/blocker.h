#pragma once
#include "check_manager.h"
#include "network_manager.h"
#include "session_manager.h"
#include <map>
#include <memory>
#include <string>

class Blocker {
    SessionManager ses_mng;

public:
    Blocker(std::string file, std::string in, std::string out) {
        NetworkManager::getInstance().setInterface(in, out);

        CheckManager::getInstance().update(file);
    }

    void start() {
        while (true) {
            std::unique_ptr<TcpData> data = NetworkManager::getInstance().recv();

            if (!data)
                continue;

            ses_mng.add(move(data));

        }

    }

};

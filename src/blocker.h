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
    Blocker(std::string file, std::string in_if, std::string cli_if, std::string srv_if) {
        NetworkManager::getInstance().setInterface(in_if, cli_if, srv_if);

        CheckManager::getInstance().update(file);
    }

    void start() {
        LogManager::getInstance().log("Block Start");
        while (true) {

            //NetworkManager::getInstance();

            std::unique_ptr<TcpData> data = NetworkManager::getInstance().recv(); // Seg fault

            if (!data)
                continue;

            ses_mng.add(move(data));

        }

    }

};

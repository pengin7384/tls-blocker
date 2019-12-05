#pragma once
#include <map>
#include <memory>
#include <string>
#include "check_manager.h"
#include "network_manager.h"
#include "session_manager.h"

class Blocker {
    //NetworkManager net_mng;
    SessionManager ses_mng;

public:
    Blocker(std::string in, std::string out)
        //: net_mng(in, out)
    {
        NetworkManager::getInstance().setInterface(in, out);

        // 싱글톤 테스트해야함!
        CheckManager::getInstance().test();
        CheckManager::getInstance().test();
        CheckManager::getInstance().test();
        CheckManager::getInstance().test();


    }

    void start() {
        LogManager::getInstance().log("start\n");

        while (true) {
            std::unique_ptr<TcpData> data = NetworkManager::getInstance().recv();

            if (!data)
                continue;

            ses_mng.add(move(data));

        }

    }

};

#pragma once

#include <functional>

#include <memory>
#include <pcap/pcap.h>
#include <string>
#include "log_manager.h"

class NetworkManager {
    char err_buf[PCAP_ERRBUF_SIZE];
    std::shared_ptr<pcap_t> in_handle;
    std::shared_ptr<pcap_t> out_handle;

public:
    NetworkManager(std::string in, std::string out)
    {
        if (in.empty() || out.empty()) {
            LogManager::getInstance().log("Interface Error");
            return;
        }

        in_handle.reset(pcap_open_live(in.c_str(), BUFSIZ, 1, 1, err_buf), pcap_close);
        out_handle.reset(pcap_open_live(out.c_str(), BUFSIZ, 1, 1, err_buf), pcap_close);

        if (in_handle == nullptr || out_handle == nullptr) {
            LogManager::getInstance().log("Error while opening handle");
            return;
        }
    }

    ~NetworkManager() = default;
};

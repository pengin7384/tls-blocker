#pragma once

#include <cstdint>
#include <cstring>
#include <netinet/ether.h>
#include <linux/if.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>

struct EtherAddr
{
    uint8_t host[ETH_ALEN];
    EtherAddr() {

    }

    EtherAddr(const uint8_t *host) {
        setHost(host);
    }

    EtherAddr(std::string interface) {
        struct ifreq ifr;
        std::memset(&ifr, 0, sizeof(ifr));
        std::strncpy(ifr.ifr_name, interface.c_str(), interface.length());
        int sock = socket(AF_UNIX, SOCK_DGRAM, 0);

        if (sock < 0 || ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
            return;
        }
        close(sock);
        setHost(reinterpret_cast<const uint8_t *>(ifr.ifr_hwaddr.sa_data));
    }

    void setHost(const uint8_t *host) {
        for (int i = 0; i < ETH_ALEN; i++) {
            this->host[i] = host[i];
        }
    }
};

#pragma once
#include <arpa/inet.h>

struct SockAddr
{
    in_addr_t ip;
    in_port_t port;

    SockAddr() {

    }

    SockAddr(in_addr_t ip, in_port_t port)
        : ip(ip),
          port(port) {

    }

    bool operator<(const SockAddr &sock_addr) const {
        return port < sock_addr.port || ip < sock_addr.ip;
    }
};

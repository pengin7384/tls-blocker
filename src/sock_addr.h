#pragma once
#include <arpa/inet.h>
#include <unordered_map>

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

    bool operator==(const SockAddr& other) const {
        return ip==other.ip && port==other.port;
    }

    bool operator<(const SockAddr &sock_addr) const {
        return port < sock_addr.port || ip < sock_addr.ip;
    }
};

namespace  std {

template <>
struct hash<SockAddr> {

    std::size_t operator()(const SockAddr& s) const {
        return hash<in_addr_t>()(s.ip) ^ hash<in_port_t>()(s.port);
    }
};

}

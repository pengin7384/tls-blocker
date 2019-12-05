#pragma once
#include "ether_addr.h"
#include "log_manager.h"
#include "sock_addr.h"
#include <vector>

struct TcpData {
    EtherAddr src_ether;
    EtherAddr dst_ether;
    SockAddr src_sock;
    SockAddr dst_sock;
    std::vector<uint8_t> payload;
    bool tcp_syn;
    uint32_t tcp_seq;
    uint32_t tcp_ack;

    TcpData(EtherAddr src_ether,
            EtherAddr dst_ether,
            SockAddr src_sock,
            SockAddr dst_sock,
            std::vector<uint8_t> payload,
            bool tcp_syn,
            uint32_t tcp_seq,
            uint32_t tcp_ack)
        : src_ether(src_ether),
          dst_ether(dst_ether),
          src_sock(src_sock),
          dst_sock(dst_sock),
          payload(payload),
          tcp_syn(tcp_syn),
          tcp_seq(tcp_seq),
          tcp_ack(tcp_ack)
    {

    }
};



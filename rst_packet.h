#pragma once
#include "network_header.h"
#include <cstring>
#include <memory>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

typedef struct ether_header EthernetHeader;
typedef struct iphdr IPHeader;
typedef struct tcphdr TCPHeader;

#pragma pack(1)
struct Rst {
    EthernetHeader eth_hdr;
    IPHeader ip_hdr;
    TCPHeader tcp_hdr;

};
#pragma pack()

class RstPacket {
    std::shared_ptr<Rst> rst;

public:
    RstPacket() {
        rst = std::make_shared<Rst>();

        /* Ethernet */
        std::memset(rst.get()->eth_hdr.ether_dhost, 0, ETH_ALEN);
        std::memset(rst.get()->eth_hdr.ether_shost, 0, ETH_ALEN);
        rst.get()->eth_hdr.ether_type = htons(ETHERTYPE_IP);

        /* IPv4 */
        rst.get()->ip_hdr.ihl = sizeof(IPHeader) / 4;
        rst.get()->ip_hdr.version = IPVERSION;
        rst.get()->ip_hdr.tos = IPTOS_ECN_NOT_ECT;
        rst.get()->ip_hdr.tot_len = htons(sizeof(IPHeader) + sizeof(TCPHeader));
        rst.get()->ip_hdr.id = 0;
        rst.get()->ip_hdr.frag_off = htons(IP_DF);
        rst.get()->ip_hdr.ttl = IPDEFTTL;
        rst.get()->ip_hdr.protocol = IPPROTO_TCP;
        rst.get()->ip_hdr.check = 0;
        rst.get()->ip_hdr.saddr = 0;
        rst.get()->ip_hdr.daddr = 0;

        /* TCP */
        rst.get()->tcp_hdr.th_sport = 0;
        rst.get()->tcp_hdr.th_dport = 0;
        rst.get()->tcp_hdr.th_seq = 0;
        rst.get()->tcp_hdr.th_ack = 0;
        rst.get()->tcp_hdr.th_x2 = 0;
        rst.get()->tcp_hdr.th_off = sizeof(TCPHeader) / 4;
        rst.get()->tcp_hdr.th_flags = TH_RST;
        rst.get()->tcp_hdr.th_win = 0;
        rst.get()->tcp_hdr.th_sum = 0;
        rst.get()->tcp_hdr.th_urp = 0;
    }

    uint16_t calcCheckSum(uint16_t *data, int len) {
        uint16_t result = 0;
        int i = len / 2;

        if (len & 1)
            result = ntohs(data[i] & 0x00ff);

        while (i--) {
            if (0xffff - ntohs(data[i]) < result)
                result++;
            result += ntohs(data[i]);
        }

        return result;
    }

    uint16_t getTcpCheckSum(IPHeader *ip, TCPHeader *tcp) {
        // Pseudo Header
        struct
        {
            in_addr_t saddr;
            in_addr_t daddr;
            uint8_t reserved = 0;
            uint8_t protocol = IPPROTO_TCP;
            uint16_t tcpLen;
        } ps_hdr;

        ps_hdr.saddr = ip->saddr;
        ps_hdr.daddr = ip->daddr;
        ps_hdr.tcpLen = htons(ntohs(ip->tot_len) - static_cast<uint16_t>((ip->ihl << 2)));

        // pseudoChecksum
        uint16_t ps_chksum = calcCheckSum(reinterpret_cast<uint16_t *>(&ps_hdr), sizeof(ps_hdr));

        // TCP Segement Checksum
        uint16_t tcp_chksum = calcCheckSum(reinterpret_cast<uint16_t *>(tcp), ntohs(ps_hdr.tcpLen));

        uint16_t chksum = ps_chksum;
        if (0xffff - tcp_chksum < chksum)
            chksum++;
        chksum += tcp_chksum;

        return ~chksum;
    }

    uint16_t getIpCheckSum(IPHeader *ip) {
        return ~calcCheckSum(reinterpret_cast<uint16_t *>(ip), ip->ihl * 4);
    }

    void setSrcEther(const EtherAddr &ether) {
        std::memcpy(rst.get()->eth_hdr.ether_shost, ether.host, ETH_ALEN);
    }

    void change(const SockAddr &src_sock, const SockAddr &dst_sock, const EtherAddr &dst_ether, uint32_t seq) {
        std::memcpy(rst.get()->eth_hdr.ether_dhost, dst_ether.host, ETH_ALEN);
        rst.get()->ip_hdr.saddr = htonl(src_sock.ip);
        rst.get()->ip_hdr.daddr = htonl(dst_sock.ip);
        rst.get()->ip_hdr.check = 0;
        rst.get()->tcp_hdr.check = 0;

        rst.get()->ip_hdr.check = htons(getIpCheckSum(&rst.get()->ip_hdr));

        rst.get()->tcp_hdr.th_sport = htons(src_sock.port);
        rst.get()->tcp_hdr.th_dport = htons(dst_sock.port);
        rst.get()->tcp_hdr.th_seq = htonl(seq);

        rst.get()->tcp_hdr.th_sum = htons(getTcpCheckSum(&rst.get()->ip_hdr, &rst.get()->tcp_hdr));
    }

    uint8_t *getRaw() {
        return reinterpret_cast<uint8_t*>(rst.get());
    }

};

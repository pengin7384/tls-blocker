#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <netinet/ether.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <pcap/pcap.h>
#include <string>
#include <vector>
#include "log_manager.h"
#include "network_header.h"

typedef struct ether_header EthernetHeader;
typedef struct iphdr IPHeader;
typedef struct tcphdr TCPHeader;

class NetworkManager {
    char err_buf[PCAP_ERRBUF_SIZE];
    std::shared_ptr<pcap_t> in_handle;
    std::shared_ptr<pcap_t> out_handle;
    std::mutex in_mtx;
    std::mutex out_mtx;

public:
    NetworkManager(std::string in, std::string out)
    {
        /* Open pcap handle */
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

    std::unique_ptr<TcpData> recv() {
        std::lock_guard<std::mutex> guard(in_mtx);
        pcap_pkthdr *header = nullptr;
        const EthernetHeader *eth_header = nullptr;
        const IPHeader *ip_header = nullptr;
        const uint8_t *packet = nullptr;

        for (int res = 0; res == 0;) {
            res = pcap_next_ex(in_handle.get(), &header, &packet);
            if (res < 0)
                return nullptr;

            eth_header = reinterpret_cast<const EthernetHeader *>(packet);
            ip_header = reinterpret_cast<const IPHeader *>(packet + sizeof(EthernetHeader));

            if (ntohs(eth_header->ether_type) != ETHERTYPE_IP ||
                    ip_header->protocol != IPPROTO_TCP) {
                res = 0;
            }

        }

        const TCPHeader *tcp_header = reinterpret_cast<const TCPHeader *>(packet
                                                                   + sizeof(EthernetHeader)
                                                                   + (ip_header->ihl * 4));

        if (ntohs(tcp_header->dest) != 443) {
            return nullptr;
        }

        const uint8_t *payload = reinterpret_cast<const uint8_t *>(packet
                                                                   + sizeof(EthernetHeader)
                                                                   + (ip_header->ihl * 4)
                                                                   + (tcp_header->doff * 4));
        const uint32_t payload_len = ntohs(ip_header->tot_len) - (ip_header->ihl * 4) - (tcp_header->doff * 4);

        std::unique_ptr<TcpData> ptr = std::make_unique<TcpData>(eth_header->ether_shost,
                                                                 eth_header->ether_dhost,
                                                                 SockAddr(ntohl(ip_header->saddr), ntohs(tcp_header->source)),
                                                                 SockAddr(ntohl(ip_header->daddr), ntohs(tcp_header->dest)),
                                                                 std::vector<uint8_t>(payload, payload + payload_len),
                                                                 bool(tcp_header->syn),
                                                                 ntohl(tcp_header->seq),
                                                                 ntohl(tcp_header->ack_seq));
        return ptr;
    }

    void send() {
        std::lock_guard<std::mutex> guard(out_mtx);
    }
};



#pragma once
#include "log_manager.h"
#include "network_header.h"
#include "rst_packet.h"
#include "singleton.h"
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

typedef struct ether_header EthernetHeader;
typedef struct iphdr IPHeader;
typedef struct tcphdr TCPHeader;

class NetworkManager : public Singleton<NetworkManager>  {
    char err_buf[PCAP_ERRBUF_SIZE];
    std::shared_ptr<pcap_t> in_handle;
    std::shared_ptr<pcap_t> out_handle;
    std::mutex in_mtx;
    std::mutex out_mtx;
    RstPacket rst;
    EtherAddr out_ether;

public:
    ~NetworkManager() {

    }

    void setInterface(std::string in, std::string out)
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

        out_ether = EtherAddr(out);
        rst.setSrcEther(out_ether);
    }

    std::unique_ptr<TcpData> recv() {
        //std::lock_guard<std::mutex> guard(in_mtx);

        if (!in_handle) {
            LogManager::getInstance().log("Error : Input Interface is nullptr");
            return nullptr;
        }

        pcap_pkthdr *header = nullptr;
        const EthernetHeader *eth_header = nullptr;
        const IPHeader *ip_header = nullptr;
        const uint8_t *packet = nullptr;

        for (int res = 0; res == 0;) {
            std::lock_guard<std::mutex> guard(in_mtx);
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

    void sendRstPacket(const SockAddr &src_sock, const SockAddr &dst_sock, const EtherAddr &src_ether, const EtherAddr &dst_ether, uint32_t start_seq, uint32_t last_ack, uint16_t len) {
        //std::lock_guard<std::mutex> guard(out_mtx);

        if (!out_handle) {
            LogManager::getInstance().log("Error : Output Interface is nullptr");
            return;
        }

        /* Server */
        rst.change(src_sock, dst_sock, dst_ether, start_seq + 1 + len);
//        LogManager::getInstance().log("Client -> server");
//        for (size_t i = 0; i < sizeof(Rst); i++) {
//            if (i % 16 == 0 && i != 0) {
//                puts("");
//            }
//            printf("%02x ", rst.getRaw()[i]);
//        }
//        puts("\n");

        pcap_sendpacket(out_handle.get(), rst.getRaw(), sizeof(Rst));
        rst.change(dst_sock, src_sock, src_ether, last_ack);
//        LogManager::getInstance().log("Server -> client");
//        for (size_t i = 0; i < sizeof(Rst); i++) {
//            if (i % 16 == 0 && i != 0) {
//                puts("");
//            }
//            printf("%02x ", rst.getRaw()[i]);
//        }
//        puts("\n");


        pcap_sendpacket(out_handle.get(), rst.getRaw(), sizeof(Rst));
//        LogManager::getInstance().log("RST Sent");

    }
};



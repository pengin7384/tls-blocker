#pragma once
#include "log_manager.h"
#include "network_header.h"
#include "rst_packet.h"
#include "singleton.h"
#include <cstdint>
#include <functional>
#include <memory>
#include <pcap/pcap.h>
#include <string>
#include <vector>

#define MAX_SIZE 8192

class NetworkManager : public Singleton<NetworkManager>  {
    char err_buf[PCAP_ERRBUF_SIZE];
    const char *tcp_ssl_filter = "ip proto TCP and dst port 443";
    std::shared_ptr<pcap_t> in_handle;
    std::shared_ptr<pcap_t> cli_handle;
    std::shared_ptr<pcap_t> srv_handle;
    std::mutex out_mtx;
    EtherAddr cli_ether;
    EtherAddr srv_ether;

public:
    ~NetworkManager() {

    }

    void setInterface(std::string in_if, std::string cli_if, std::string srv_if)
    {
        /* Open pcap handle */
        if (in_if.empty() || cli_if.empty() || srv_if.empty()) {
            LogManager::getInstance().log("Interface Error");
            return;
        }

        in_handle.reset(pcap_open_live(in_if.c_str(), MAX_SIZE, 1, 1, err_buf), pcap_close);
        cli_handle.reset(pcap_open_live(cli_if.c_str(), MAX_SIZE, 1, 1, err_buf), pcap_close);
        srv_handle.reset(pcap_open_live(srv_if.c_str(), MAX_SIZE, 1, 1, err_buf), pcap_close);

        if (in_handle.get() == nullptr || cli_handle.get() == nullptr || srv_handle.get() == nullptr) {
            LogManager::getInstance().log("Error while opening handle");
            return;
        }

        /* Filter */
        bpf_program bpf;
        if (pcap_compile(in_handle.get(), &bpf, tcp_ssl_filter, 1, 0) < 0) {
            LogManager::getInstance().log("Error while compiling filter");
            return;
        }

        if (pcap_setfilter(in_handle.get(), &bpf) < 0) {
            LogManager::getInstance().log("Error while applying filter");
            return;
        }

        cli_ether = EtherAddr(cli_if);
        srv_ether = EtherAddr(srv_if);
    }

    std::unique_ptr<TcpData> recv() {
        pcap_pkthdr *header = nullptr;
        const ether_header *eth_header = nullptr;
        const iphdr *ip_header = nullptr;
        const uint8_t *packet = nullptr;

        for (int res = 0; res == 0;) {
            res = pcap_next_ex(in_handle.get(), &header, &packet);
            if (res < 0)
                return nullptr;

            if (res == 0)
                continue;

            eth_header = reinterpret_cast<const ether_header *>(packet);
            ip_header = reinterpret_cast<const iphdr *>(packet + sizeof(ether_header));
        }

        const tcphdr *tcp_header = reinterpret_cast<const tcphdr *>(packet
                                                                   + sizeof(ether_header)
                                                                   + (ip_header->ihl * 4));

        if (ntohs(tcp_header->dest) != 443) {
            return nullptr;
        }

        const uint8_t *payload = reinterpret_cast<const uint8_t *>(packet
                                                                   + sizeof(ether_header)
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
        RstPacket rst;

        /* Server */
        rst.setSrcEther(srv_ether);
        rst.change(src_sock, dst_sock, dst_ether, start_seq + 1 + len);
        int srv_res = pcap_sendpacket(srv_handle.get(), rst.getRaw(), sizeof(Rst));
        rst.changeSeq(start_seq + 1 + len + 1600);
        srv_res = pcap_sendpacket(srv_handle.get(), rst.getRaw(), sizeof(Rst));

        /* Client */
        rst.setSrcEther(cli_ether);
        rst.change(dst_sock, src_sock, src_ether, last_ack);
        int cli_res = pcap_sendpacket(cli_handle.get(), rst.getRaw(), sizeof(Rst));
        rst.changeSeq(last_ack + 1600);
        cli_res = pcap_sendpacket(cli_handle.get(), rst.getRaw(), sizeof(Rst));

        if (srv_res != 0) {
            LogManager::getInstance().log("Error while sending rst to server");
        }

        if (cli_res != 0) {
            LogManager::getInstance().log("Error while sending rst to client");
        }



    }
};



#pragma once
#include "check_manager.h"
#include "mutex_queue.h"
#include "network_header.h"
#include "network_manager.h"
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>

#define BUF_SIZE 1600


/**
 * @brief The Result enum
 * 'complete'   : Finished reassembling
 * 'wait'       : Not completed
 * 'error'      : Error while reassembling
 * 'ignore'     : Not a target packet
 */
enum class Result { complete, wait, error, ignore };

class Session {
    typedef std::function<void(SockAddr)> funcErase;

    std::atomic<bool> die_flag;
    std::vector<uint8_t> payload;
    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> que;

    funcErase callback;
    SockAddr src_sock;
    SockAddr dst_sock;
    EtherAddr src_ether;
    EtherAddr dst_ether;

    /* Reassemble */
    uint32_t start_seq;
    uint32_t last_ack;
    std::set<uint32_t> seq_set;
    std::bitset<5> flag_set;
    int64_t target_len;

    uint32_t cnt;

    int64_t recv_cnt;
    std::atomic<bool> block_flag;

public:
    Session() = delete;

    Session(std::unique_ptr<TcpData> ptr, funcErase callback) : die_flag(false)
    {
        payload.reserve(BUF_SIZE);
        que.reset(new MutexQueue<std::unique_ptr<TcpData>>(), freeQueue<std::unique_ptr<TcpData>>);
        this->callback = callback;
        src_sock = ptr.get()->src_sock;
        dst_sock = ptr.get()->dst_sock;
        src_ether = ptr.get()->src_ether;
        dst_ether = ptr.get()->dst_ether;
        start_seq = ptr.get()->tcp_seq;
        target_len = -1;
        cnt = 0;
        recv_cnt = 0;

        block_flag.store(false);
    }

    ~Session() {
    }

    template <typename T>
    static void freeQueue(MutexQueue<T> *target) {

    }

    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> getQueue() {
        return que;
    }

    bool checkBlock() {
        return block_flag.load();
    }

    Result reassemble(std::unique_ptr<TcpData> data) {

        if (data.get()->payload.size() == 0) {
            return Result::wait;
        }

        uint32_t rel_seq_num = data.get()->tcp_seq - start_seq;
        uint32_t data_index = rel_seq_num - 1;


        if ((data_index + data->payload.size()) > payload.size()) {
            size_t tot_size = data_index + payload.size() + data->payload.size();
            if(tot_size > 8200) {
                return Result::ignore;
            }
            payload.resize(tot_size);
        }


        auto it = seq_set.find(data.get()->tcp_seq);
        if (it != seq_set.end()) {
            LogManager::getInstance().log("Already reassembled");
            return Result::wait;
        }

        seq_set.insert(data.get()->tcp_seq);
        last_ack = data.get()->tcp_ack;

        for (size_t i = 0; i < data.get()->payload.size(); i++) {
            payload.at(data_index + i) = data.get()->payload.at(i);
        }
        recv_cnt += data.get()->payload.size();

        /* Flag */
        size_t index = data_index;
        while (true) {
            if (index > 4 || index >= (data_index + data.get()->payload.size())) {
                break;
            }

            flag_set[index] = 1;
            index++;

            if (flag_set[0] && payload.at(0) != 0x16)
                return Result::ignore;

            if (flag_set[1] && payload.at(1) != 0x03)
                return Result::ignore;

            if (flag_set[2] && payload.at(2) != 0x01)
                return Result::ignore;

            if (flag_set[3] && flag_set[4]) {
                uint16_t len = payload.at(3) * 256 + payload.at(4) + 5;
                target_len = len;
            }
        }

        if (target_len == -1) {
            return Result::wait;
        } else if (recv_cnt < target_len) {
            return Result::wait;
        } else if (recv_cnt == target_len) {
            return Result::complete;
        } else {
//            block_flag.store(true);
//            LogManager::getInstance().log("Error: RecvLen > TargetLen");
//            printf("recv:%lu, target:%lu (%u)\n", recv_cnt, target_len, src_sock.port);
//            usleep(1000);
            return Result::complete;
            return Result::error;
        }

    }

    int changeSupportedVersions() {
        uint32_t len = payload[5 + 1] * 256 * 256 + payload[5 + 2] * 256 + payload[5 + 3];
        uint32_t index = 0;

        /* Session Length */
        index += 39 + payload[5 + 38];

        /* Cipher Suites Length */
        index += 2 + payload[5 + index] * 256 + payload[5 + index + 1];

        /* Compression Methods Length */
        index += 1 + payload[5 + index];

        /* Extensions Length */
        index += 2;

        bool change_version = false;

        while (index < len) {
            /* Supported Versions */
            if (payload[5 + index] == 0x00 && payload[5 + index + 1] == 0x2b) {
                uint16_t len = payload[5 + index + 2] * 256 + payload[5 + index + 3];
                for (uint16_t i = 0; i < len; i+=2) {
                    if (payload[5 + index + 5 + i] == 0x03 && payload[5 + index + 5 + i + 1] == 0x04) {
                        payload[5 + index + 5 + i + 1] = 0x03;
                        change_version = true;
                    }
                }
            }
            /* ESNI */
            else if (payload[5 + index] == 0xff && payload[5 + index + 1] == 0xce) {
                if (change_version) {
//                    payload[5 + index + 66] = 0x1;
//                    payload[5 + index + 67] = 0x2;
                    return 1;
                }
            }

            index += 2;

            index += 2 + payload[5 + index] * 256 + payload[5 + index + 1];
        }
        return 0;
    }

    /* TODO: change style */
    std::string getServerName() {
        uint32_t len = payload[5 + 1] * 256 * 256 + payload[5 + 2] * 256 + payload[5 + 3];
        uint32_t index = 0;

        /* Session Length */
        index += 39 + payload[5 + 38];

        /* Cipher Suites Length */
        index += 2 + payload[5 + index] * 256 + payload[5 + index + 1];

        /* Compression Methods Length */
        index += 1 + payload[5 + index];

        /* Extensions Length */
        index += 2;

        while (index < len) {
            /* SNI */
            if (payload[5 + index] == 0x00 && payload[5 + index + 1] == 0x00) {
                uint16_t serv_name_len = payload[5 + index + 7] * 256 + payload[5 + index + 8];

                std::string serv_name = "";
                for (uint16_t i = 0; i < serv_name_len; i++) {
                    serv_name += static_cast<char>(payload[5 + index + 9 + i]);
                }
                return serv_name;
            }

            index += 2;

            index += 2 + payload[5 + index] * 256 + payload[5 + index + 1];
        }
        return "";
    }

    int findDomain(const uint8_t *data, size_t data_len, const char *target) {
        for (size_t i = 0; i < data_len; i++) {
            size_t j = 0;
            bool flag = false;
            while (target[j] != 0) {
                if ((i + j) >= data_len) {
                    break;;
                }
                if (data[i + j] != target[j]) {
                    break;;
                }
                j++;
                if (target[j] == 0) {
                    flag = true;
                    break;
                }
            }
            if (flag == true) {
                return 1;
            }

        }
        return 0;
    }

    void process() {

        while (true) {
            if (die_flag.load()) {
                callback(src_sock);
                break;
            }

            /* TODO: Change cnt to time */
            if (cnt++ > 20000000) {
                LogManager::getInstance().log("Time out " + std::string());
                kill();
                continue;
            }


            if (que.get()->size() == 0) {
                continue;
            }

            std::unique_ptr<TcpData> data = que.get()->front();
            que.get()->pop();
            Result res = reassemble(move(data));

            if (res == Result::complete) {
//                std::string server_name = getServerName();

//                if (CheckManager::getInstance().isBlocked(server_name)) {

//                    NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
//                                                                dst_ether, start_seq, last_ack,
//                                                                static_cast<uint16_t>(payload.size()));
//                    LogManager::getInstance().log("Server name : (" + server_name + ") Blocked " + std::to_string(src_sock.port));
//                } else {
//                    LogManager::getInstance().log("Server name : (" + server_name + ") is not Blocked");
//                }
                int esni_check = changeSupportedVersions();

                if (esni_check == 1) {
                    /* Send client hello to server */

                    int sock = socket(PF_INET, SOCK_STREAM, 0);
                    if (sock == -1) {
                        LogManager::getInstance().log("socket error");
                    }

                    sockaddr_in addr;
                    addr.sin_family = AF_INET;
                    addr.sin_port = htons(443);
                    addr.sin_addr.s_addr = htonl(dst_sock.ip);

                    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == -1) {
                        LogManager::getInstance().log("connect error");
                    }

                    size_t tot_send_size = 0;
                    while (tot_send_size < payload.size()) {
                        //printf("tot:%u, send:%u\n", tot_send_size, sen)
                        ssize_t send_size = send(sock, &payload.at(tot_send_size), payload.size() - tot_send_size, 0);
                        if (send_size == -1) {
                            LogManager::getInstance().log("send error");
                            break;

                        }
                        tot_send_size += static_cast<size_t>(send_size);
                    }

                    uint8_t buf[8000];

                    size_t tot_recv_size = 0;
                    tot_recv_size = recv(sock, buf, 8000, 0);
                    int result = findDomain(buf, tot_recv_size, "hellven.net");
                    //int result = 1;
                    if (result == 1) {
                        block_flag.store(true); // Notice to session_manager

                        uint32_t test_ack;
                        uint16_t test_len;

                        if (que->empty()) {
                            test_ack = last_ack;
                            test_len = static_cast<uint16_t>(payload.size());
                        } else {
                            std::unique_ptr<TcpData> last_data = que.get()->back();
                            test_ack = last_data.get()->tcp_ack;
                            test_len = static_cast<uint16_t>(last_data.get()->tcp_seq - start_seq + last_data.get()->payload.size() - 1);
                        }

                        NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
                                                                    dst_ether, start_seq, test_ack,
                                                                    test_len);
                        printf("send rst!!(%u)\n", src_sock.port);
                        printf("hellven.net blocked!\n");
                        //                    NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
                        //                                                                dst_ether, start_seq, last_ack,
                        //                                                                static_cast<uint16_t>(payload.size()));
                        usleep(100 * 1000);
                    }

                }


                kill();
            } else if (res == Result::ignore) {
                kill();
            } else if (res == Result::error) {
                kill();
            } else if (res == Result::wait) {

            }
        }
    }

    void sendRst(std::unique_ptr<TcpData> data) {

        NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
                                                    dst_ether, start_seq, data.get()->tcp_ack,
                                                    static_cast<uint16_t>(data.get()->tcp_seq - start_seq + data.get()->payload.size() - 1));

//        uint32_t test_ack;
//        uint16_t test_len;

//        if (que->empty()) {
//            test_ack = last_ack;
//            test_len = static_cast<uint16_t>(payload.size());
//        } else {
//            std::unique_ptr<TcpData> last_data = que.get()->back();
//            test_ack = last_data.get()->tcp_ack;
//            test_len = static_cast<uint16_t>(last_data.get()->tcp_seq - start_seq + last_data.get()->payload.size() - 1);
//        }

//        NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
//                                                    dst_ether, start_seq, test_ack,
//                                                    test_len);
    }

    void kill() {
        die_flag.store(true);
    }

    bool getDieFlag() {
        return die_flag.load();
    }

};

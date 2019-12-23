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
    std::atomic<bool> block_flag;
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

    //uint32_t cnt;
    std::chrono::system_clock::time_point end_time = std::chrono::system_clock::now() + std::chrono::seconds(10);
    int64_t recv_cnt;

public:
    Session() = delete;

    Session(std::unique_ptr<TcpData> ptr, funcErase callback) : die_flag(false)
    {
        block_flag.store(false);
        payload.reserve(BUF_SIZE);
        que.reset(new MutexQueue<std::unique_ptr<TcpData>>(), freeQueue<std::unique_ptr<TcpData>>);
        this->callback = callback;
        src_sock = ptr.get()->src_sock;
        dst_sock = ptr.get()->dst_sock;
        src_ether = ptr.get()->src_ether;
        dst_ether = ptr.get()->dst_ether;
        start_seq = ptr.get()->tcp_seq;
        target_len = -1;
        recv_cnt = 0;
    }

    ~Session() {
    }

    uint32_t getStartSeq()
    {
        return start_seq;
    }

    template <typename T>
    static void freeQueue(MutexQueue<T> *target) {
        delete target;
    }

    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> getQueue() {
        return que;
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
            LogManager::getInstance().log("Error: RecvLen > TargetLen");
            return Result::error;
        }

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

    void process() {

        while (true) {
            /* Kill */
            if (die_flag.load()) {
                callback(src_sock);
                break;
            }

            /* Timeout */
            if (que.get()->empty()) {
                if (std::chrono::system_clock::now() >= end_time) {
                    kill();
                    LogManager::getInstance().log("Time out");
                } else {
                    usleep(10);
                }
                continue;
            }

            /* Reassemble */
            std::unique_ptr<TcpData> data = que.get()->front();
            que.get()->pop();
            Result res = reassemble(move(data));

            if (res == Result::complete) {
                std::string server_name = getServerName();

                if (CheckManager::getInstance().isBlocked(server_name)) {

                    block_flag.store(true);
                    NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
                                                                dst_ether, start_seq, last_ack,
                                                                static_cast<uint16_t>(payload.size()));
                    LogManager::getInstance().log("Server name : (" + server_name + ") Blocked " + std::to_string(src_sock.port));
                    sleep(3);
                } else {
                    LogManager::getInstance().log("Server name : (" + server_name + ") is not Blocked");
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

    void sendRst(std::unique_ptr<TcpData> data)
    {
        NetworkManager::getInstance().sendRstPacket(src_sock, dst_sock, src_ether,
            dst_ether, start_seq, data->tcp_ack,
            static_cast<uint16_t>(data->tcp_seq - start_seq + data->payload.size() - 1));
    }

    void kill() {
        die_flag.store(true);
    }

    bool getBlockFlag()
    {
        return block_flag.load();
    }

    bool getDieFlag() {
        return die_flag.load();
    }

};

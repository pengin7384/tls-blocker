#pragma once

#include <atomic>
#include <bitset>
#include <cstdint>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include "mutex_queue.h"
#include "network_header.h"

#define BUF_SIZE 1600

enum class Result { complete, success, error, ignore };

class Session {
    typedef std::function<void(SockAddr)> funcErase;

    std::atomic<bool> die_flag;
    std::vector<uint8_t> payload;
    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> que;

    funcErase callback;
    SockAddr src_sock;

    /* Reassemble */
    uint32_t start_seq;
    uint32_t last_ack;
    std::set<uint32_t> seq_set;
    std::bitset<5> flag_set;
    int64_t target_len;

public:
    Session() = delete;

    Session(std::unique_ptr<TcpData> ptr, funcErase callback) : die_flag(false)
    {
        payload.reserve(BUF_SIZE);
        //payload.assign(BUF_SIZE, 0);
        que.reset(new MutexQueue<std::unique_ptr<TcpData>>(), freeQueue<std::unique_ptr<TcpData>>);
        this->callback = callback;
        src_sock = ptr.get()->src_sock;
        start_seq = ptr.get()->tcp_seq;
        target_len = -1;
    }

    template <typename T>
    static void freeQueue(MutexQueue<T> *target) {

    }

    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> getQueue() {
        return que;
    }


    Result reassemble(std::unique_ptr<TcpData> data) {

        if (data.get()->payload.size() == 0) {
            return Result::success;
        }

        uint32_t rel_seq_num = data.get()->tcp_seq - start_seq;
        uint32_t data_index = rel_seq_num - 1;

        if ((data_index + data->payload.size()) > payload.size()) {
            //LogManager::getInstance().log("Payload resized");
            payload.resize(payload.size() + data->payload.size());
        }


        auto it = seq_set.find(data.get()->tcp_seq);
        if (it != seq_set.end()) {
            LogManager::getInstance().log("Already reassembled");
            return Result::success;
        }

        seq_set.insert(data.get()->tcp_seq);
        last_ack = data.get()->tcp_ack;

        for (size_t i = 0; i < data.get()->payload.size(); i++) {
            payload.at(data_index + i) = data.get()->payload.at(i);
        }

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
            return Result::success;
        } else if (static_cast<int64_t>(payload.size()) < target_len) {
            return Result::success;
        } else if (static_cast<int64_t>(payload.size()) == target_len) {
            LogManager::getInstance().log("Reassembled");
            return Result::complete;
        } else {
            LogManager::getInstance().log("Error: RecvLen > TargetLen");
            return Result::error;
        }

    }

    void process() {
        //callback(src_sock);
        while (true) {
            if (die_flag.load()) {
                callback(src_sock);
                break;
            }

            /* TODO: Timeout */
            if (que.get()->size() == 0) {
                continue;
            }

            std::unique_ptr<TcpData> data = que.get()->front();
            que.get()->pop();
            Result res = reassemble(move(data));

            if (res == Result::complete) {
                break;

            } else if (res == Result::ignore || res == Result::error) {
                LogManager::getInstance().log("ignore || error");
                break;
            } else if (res == Result::success) {

            }

            // If ignore || error then drop session

        }
    }


    void kill() {
        die_flag.store(true);
    }

    bool getDieFlag() {
        return die_flag.load();
    }

};

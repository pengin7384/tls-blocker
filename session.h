#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "mutex_queue.h"
#include "network_header.h"

#define BUF_SIZE 1600

class Session {
    std::atomic<bool> die_flag;
    std::vector<uint8_t> payload;
    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> que;

public:
    Session() = delete;

    Session(std::unique_ptr<TcpData> ptr) : die_flag(false)
    {
        payload.assign(BUF_SIZE, 0);
        que.reset(new MutexQueue<std::unique_ptr<TcpData>>(), freeQueue<std::unique_ptr<TcpData>>);
    }

    template <typename T>
    static void freeQueue(MutexQueue<T> *target) {

    }

    std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> getQueue() {
        return que;
    }

    void process() {


    }

//    static void process(std::shared_ptr<Session> ptr) {
//        while (1) {
//            if (ptr.get()->getDieFlag() == true) {
//                /* TODO : Free memory */
//                break;
//            }

//            /* TODO : Timeout */
//            auto


//            if (!que->empty()) {
//                //std::unique_ptr<TcpData> data = que.get()->front();




//            }




//        }
//    }

    void kill() {
        die_flag.store(true);
    }

    bool getDieFlag() {
        return die_flag.load();
    }

};

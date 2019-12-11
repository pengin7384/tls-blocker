#pragma once
#include "log_manager.h"
#include <iostream>
#include <queue>
#include <mutex>
#include <memory>

template <typename T>
class MutexQueue {
    std::queue<T> que;
    std::mutex mtx;

public:
    MutexQueue() {

    }

    void push(T ptr) {
        if (ptr == nullptr) {
            LogManager::getInstance().log("MutexQueue : Push nullptr");
            return;
        }

        std::lock_guard<std::mutex> guard(mtx);
        que.push(std::move(ptr));
    }

    T front() {
        std::lock_guard<std::mutex> guard(mtx);
        return move(que.front());
    }

//    T back() {
//        //std::lock_guard<std::mutex> guard(mtx);
//        return move(que.back());
//    }

    void pop() {
        std::lock_guard<std::mutex> guard(mtx);
        que.pop();
    }

    bool empty() {
        std::lock_guard<std::mutex> guard(mtx);
        return que.empty();
    }

    size_t size() {
        std::lock_guard<std::mutex> guard(mtx);
        return que.size();
    }

};

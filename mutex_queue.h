#pragma once
#include <iostream>
#include <queue>
#include <mutex>
#include <memory>
#include "log_manager.h"

template <typename T>
class MutexQueue {
    std::queue<std::unique_ptr<T>> que;
    std::mutex mtx;

public:
    void push(std::unique_ptr<T> &ptr) {
        if (ptr == nullptr) {
            LogManager::getInstance().log("MutexQueue : Push nullptr");
            return;
        }
        std::lock_guard<std::mutex> guard(mtx);
        que.push(move(ptr));
    }

    std::unique_ptr<T> &front() {
        std::lock_guard<std::mutex> guard(mtx);
        return que.front();
    }

    std::unique_ptr<T> &back() {
        std::lock_guard<std::mutex> guard(mtx);
        return que.back();
    }

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

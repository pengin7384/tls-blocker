#pragma once
#include <iostream>
#include <memory>
#include <mutex>

/* TODO: Seperate with main thread */
class LogManager {
    /* Singleton */
    static std::unique_ptr<LogManager> instance;
    static std::once_flag once_flag;
    LogManager() = default;
    LogManager(const LogManager&) = delete;
    LogManager& operator=(const LogManager&) = delete;

    std::mutex mtx;

public:
    /* Singleton */
    static LogManager& getInstance()
    {
        std::call_once(LogManager::once_flag, []() {
            instance.reset(new LogManager);
        });
        return *(instance.get());
    }

    void log(std::string str);

};



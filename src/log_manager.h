#pragma once
#include "singleton.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>

/* TODO: Seperate with main thread */
class LogManager : public Singleton<LogManager> {
    std::mutex mtx;

public:
    void log(std::string str)
    {
        time_t now_time;
        tm *now_tm;
        now_time = time(nullptr);
        now_tm = localtime(&now_time);

        std::lock_guard<std::mutex> guard(mtx);
        std::cout
                << "["
                << std::setfill('0')
                << std::setw(2)
                << now_tm->tm_hour
                << ":"
                << std::setw(2)
                << now_tm->tm_min
                << ":"
                << std::setw(2)
                << now_tm->tm_sec
                << "] "
                << str.c_str() << std::endl;
    }


};



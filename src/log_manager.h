#pragma once
#include "singleton.h"
#include <iostream>
#include <memory>
#include <mutex>

/* TODO: Seperate with main thread */
class LogManager : public Singleton<LogManager> {
    std::mutex mtx;

public:
    void log(std::string str)
    {
        //std::lock_guard<std::mutex> guard(mtx);
        std::cout << str.c_str() << std::endl;
    }


};



#pragma once
#include <iostream>
#include <memory>
#include <mutex>
#include "singleton.h"

/* TODO: Seperate with main thread */
class LogManager : public Singleton<LogManager> {
    std::mutex mtx;

public:
    void log(std::string str);

};



#include "log_manager.h"

void LogManager::log(std::string str)
{
    std::lock_guard<std::mutex> guard(mtx);
    std::cout << str.c_str() << std::endl;
}

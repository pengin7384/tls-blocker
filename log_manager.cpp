#include "log_manager.h"

std::unique_ptr<LogManager> LogManager::instance;
std::once_flag LogManager::once_flag;

void LogManager::log(std::string str)
{
    std::lock_guard<std::mutex> guard(mtx);
    std::cout << str.c_str() << std::endl;
}

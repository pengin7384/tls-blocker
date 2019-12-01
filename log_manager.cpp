#include "log_manager.h"

std::unique_ptr<LogManager> LogManager::instance;
std::once_flag LogManager::once_flag;

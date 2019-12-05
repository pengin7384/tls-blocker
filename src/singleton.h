#pragma once
#include <memory>
#include <mutex>

template <typename T>
class Singleton {
    static std::unique_ptr<T> instance;
    static std::once_flag once_flag;

public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static T& getInstance()
    {
        std::call_once(Singleton::once_flag, []() {
            instance.reset(new T);
        });
        return *(instance.get());
    }
protected:
    Singleton() {

    }
};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance;

template <typename T>
std::once_flag Singleton<T>::once_flag;

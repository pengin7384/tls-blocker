#pragma once
#include <map>
#include <memory>
#include <mutex>

template <typename K, typename V>
class MutexMap {
    std::map<K, V> mp;
    std::mutex mtx;

public:

    void insert(const K &key, const V value) {
        std::lock_guard<std::mutex> guard(mtx);
        mp.insert(std::make_pair(key, value));
    }

    typename std::map<K, V>::iterator find(const K &key) {
        std::lock_guard<std::mutex> guard(mtx);
        return mp.find(key);
    }

    typename std::map<K, V>::iterator erase(const typename std::map<K, V>::iterator it) {
        std::lock_guard<std::mutex> guard(mtx);
        return mp.erase(it);
    }

    typename std::map<K, V>::iterator end() {
        std::lock_guard<std::mutex> guard(mtx);
        return mp.end();
    }




};

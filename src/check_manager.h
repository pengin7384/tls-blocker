#pragma once
#include "singleton.h"
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_set>

class CheckManager : public Singleton<CheckManager> {
    std::unordered_set<std::string> block_list;

public:
    CheckManager() {
    }

    void update(std::string file) {
        if (block_list.size() > 0) {
            return;
        }

        std::ifstream ifs(file.c_str());
        if (!ifs)
            return;
        printf("------ Blocklist ------\n");
        while (true) {
            std::string item;
            std::getline(ifs, item);
            if (item.empty())
                break;
            printf("%s\n", item.c_str());
            block_list.insert(item);
        }
        printf("-----------------------\n");
        ifs.close();
    }

    bool isBlocked(std::string name) {
        return block_list.find(name) != block_list.end();
    }





};

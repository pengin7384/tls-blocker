#pragma once
#include "singleton.h"
#include <string>
#include <unordered_set>


class CheckManager : public Singleton<CheckManager> {
    std::unordered_set<std::string> block_list;

public:
    CheckManager() {
        printf("Created\n");
    }

    void test() {
        printf("test\n");
    }






};

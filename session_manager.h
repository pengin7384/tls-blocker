#pragma once
#include <map>
#include <memory>
#include <thread>
#include "network_header.h"
#include "session.h"

class SessionManager {
    //std::map<SockAddr, std::shared_ptr<MutexQueue<std::shared_ptr<TcpData>>>> ses_map;
    std::map<SockAddr, std::shared_ptr<Session>> ses_map;

public:
    void add(std::unique_ptr<TcpData> data) {

        if (data.get()->tcp_syn) {
            SockAddr src_addr = data.get()->src_sock;
            auto it = ses_map.find(src_addr);
            LogManager::getInstance().log("1");

            if (it != ses_map.end()) {
                /* TODO: Need to kill thread */
                it->second.get()->kill();
            }

            std::shared_ptr<Session> ses = std::make_shared<Session>(std::move(data));
            ses_map.insert(std::make_pair(src_addr, ses));

            std::thread thr { &Session::process, ses };
            thr.detach();


        } else {
            auto it = ses_map.find(data.get()->src_sock);

            if (it == ses_map.end()) {
                return;
            }
            //LogManager::getInstance().log("2");


            std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> ses_que = it->second->getQueue();
            ses_que.get()->push(move(data));
            //printf("que size:%ld\n", ses_que.get()->size());

        }

    }

};

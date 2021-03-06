#pragma once
#include "mutex_map.h"
#include "network_header.h"
#include "session.h"
#include <functional>
#include <map>
#include <memory>
#include <thread>

class SessionManager {
    MutexMap<SockAddr, std::shared_ptr<Session>> ses_map;

public:
    SessionManager() {
        //ses_map.reset(new MutexMap<SockAddr, std::shared_ptr<Session>>());
    }

    void callbackErase(SockAddr key) {
        auto it = ses_map.find(key);
        if (it != ses_map.end()) {
            ses_map.erase(it); // If iterator is deleted then error?
        }
    }

    void add(std::unique_ptr<TcpData> data) {
        if (data.get()->tcp_syn) {

            /* Load balancing */
            if (ses_map.size() > 30) {
                LogManager::getInstance().log("Full");
                return;
            }

            SockAddr src_addr = data.get()->src_sock;
            auto it = ses_map.find(src_addr);

            if (it != ses_map.end()) {
                /* TODO: Need to kill thread */
                if (data->tcp_seq == it->second->getStartSeq()) {
                    return;
                }

                it->second.get()->kill();

                /* TODO: Need to delete already existed session from ses_map */
                ses_map.erase(it);

            }

            std::shared_ptr<Session> ses = std::make_shared<Session>(std::move(data),
                                                                     std::bind(&SessionManager::callbackErase, this, std::placeholders::_1));

            ses_map.insert(src_addr, ses);

            /* Create thread */
            std::thread thr(&Session::process, ses);
            thr.detach();

        } else {
            auto it = ses_map.find(data.get()->src_sock);

            if (it == ses_map.end()) {
                return;
            }


            if (it->second->getBlockFlag() == true && data->payload.size() > 0) {
                /* If blocked session packet then send rst */
                it->second->sendRst(move(data));
                return;
            } else {
                std::shared_ptr<MutexQueue<std::unique_ptr<TcpData>>> ses_que = it->second->getQueue();
                ses_que.get()->push(move(data));
            }

        }

    }

};

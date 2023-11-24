#pragma once

#include "message.hpp"
#include <list>
#include <mutex>
#include <optional>

// It has to be concurrent because it will be accessed from 2 threads
class SortedList {
    std::list<BroadcastMessage> list;
    std::mutex lock;

public:
    void insert(BroadcastMessage message) {
        this->lock.lock();
        auto begin = list.begin();

        while (begin != list.end() &&
               message.get_sequence_number() > begin->get_sequence_number()) {
            begin++;
        }

        list.insert(begin, std::move(message));
        this->lock.unlock();
    }

    uint32_t lowest_sequence_number() {
        this->lock.lock();
        auto result = list.front().get_sequence_number();
        this->lock.unlock();

        return result;
    }

    bool empty() {
        this->lock.lock();
        bool empty = list.empty();
        this->lock.unlock();

        return empty;
    }

    BroadcastMessage pop_front() {
        this->lock.lock();
        BroadcastMessage bm = std::move(list.front());
        list.pop_front();
        this->lock.unlock();

        return bm;
    }
};
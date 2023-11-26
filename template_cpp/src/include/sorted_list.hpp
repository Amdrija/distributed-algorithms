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
    std::list<BroadcastMessage> to_be_delivered(BroadcastMessage message,
                                                uint32_t last_delivered) {
        std::list<BroadcastMessage> result;
        this->lock.lock();
        this->insert(std::move(message));
        uint32_t to_deliver = 0;
        while (!this->list.empty() && last_delivered + to_deliver ==
                                          this->lowest_sequence_number() - 1) {
            to_deliver++;
            result.push_back(this->pop_front());
        }

        this->lock.unlock();

        return result;
    }

private:
    uint32_t lowest_sequence_number() {
        return list.front().get_sequence_number();
    }

    bool empty() { return list.empty(); }

    BroadcastMessage pop_front() {
        BroadcastMessage bm = std::move(list.front());
        list.pop_front();

        return bm;
    }

    void insert(BroadcastMessage message) {
        auto begin = list.begin();

        while (begin != list.end() &&
               message.get_sequence_number() > begin->get_sequence_number()) {
            begin++;
        }

        list.insert(begin, std::move(message));
    }
};
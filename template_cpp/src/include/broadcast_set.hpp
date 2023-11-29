#pragma once

#include "interval_set.hpp"
#include "message.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class BroadcastSet {
private:
    std::unique_ptr<IntervalSet[]> messages;
    std::mutex lock;

public:
    BroadcastSet(uint8_t host_count)
        : messages(
              std::unique_ptr<IntervalSet[]>(new IntervalSet[host_count])) {}

    bool contains(BroadcastMessage &message) {
        uint8_t host_id = static_cast<uint8_t>(message.get_source() - 1);
        // this->lock.lock();
        bool found =
            this->messages[host_id].contains(message.get_sequence_number());
        // this->lock.unlock();

        return found;
    }

    void add(BroadcastMessage &message) {
        uint8_t host_id = static_cast<uint8_t>(message.get_source() - 1);

        // this->lock.lock();
        this->messages[host_id].insert(message.get_sequence_number());
        // this->lock.unlock();
    }
};
#pragma once

#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "message.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class BroadcastAckSet {
    std::unique_ptr<IntervalSet[]> acked;
    HostLookup lookup;
    uint8_t host_count;
    std::mutex lock;

public:
    BroadcastAckSet(HostLookup lookup)
        : acked(new IntervalSet[lookup.get_host_count() *
                                lookup.get_host_count()]),
          lookup(lookup), host_count(lookup.get_host_count()) {}

    uint8_t ack_count(BroadcastMessage &m) {
        this->lock.lock();

        uint8_t count = 0;
        for (uint8_t host : this->lookup.get_hosts()) {
            IntervalSet *acked_messages = this->get_acked(m.get_source(), host);
            if (acked_messages->contains(m.get_sequence_number())) {
                count++;
            }
        }
        this->lock.unlock();

        return count;
    }

    void ack(BroadcastMessage &m, uint8_t from) {
        this->lock.lock();
        IntervalSet *acked_messages = this->get_acked(m.get_source(), from);
        acked_messages->insert(m.get_sequence_number());
        this->lock.unlock();
    }

private:
    IntervalSet *get_acked(uint8_t source, uint8_t from) {
        return &this->acked.get()[(source - 1) * this->host_count + from - 1];
    }
};
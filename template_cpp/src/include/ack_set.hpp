#pragma once

#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "transport_message.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class AckSet {
private:
    std::mutex lock;
    std::unique_ptr<IntervalSet[]> acked_messages;
    HostLookup host_lookup;

public:
    AckSet(HostLookup host_lookup)
        : acked_messages(std::unique_ptr<IntervalSet[]>(
              new IntervalSet[host_lookup.get_host_count()])),
          host_lookup(host_lookup) {}

    bool is_acked(TransportMessage message) {
        uint8_t host_id = static_cast<uint8_t>(
            this->host_lookup.get_host_id_by_ip(message.address) - 1);
        this->lock.lock();
        bool found =
            this->acked_messages.get()[host_id].contains(message.get_id());
        this->lock.unlock();

        return found;
    }

    void ack(TransportMessage message) {
        uint8_t host_id = static_cast<uint8_t>(
            this->host_lookup.get_host_id_by_ip(message.address) - 1);

        this->lock.lock();
        this->acked_messages.get()[host_id].insert(message.get_id());
        this->lock.unlock();
    }

    uint32_t first_non_acked() {
        uint32_t min = UINT32_MAX;
        this->lock.lock();
        for (auto host : this->host_lookup.get_hosts()) {
            uint32_t first_non_acked =
                this->acked_messages[host].first_non_acked();
            if (first_non_acked < min) {
                min = first_non_acked;
            }
        }
        this->lock.unlock();

        return min;
    }
};
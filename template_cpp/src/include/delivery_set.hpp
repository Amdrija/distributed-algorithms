#pragma once

#include "host_lookup.hpp"
#include "interval_set.hpp"
#include "transport_message.hpp"
#include <mutex>
#include <unordered_map>
#include <unordered_set>

class DeliverySet {
private:
    std::mutex lock;
    std::unique_ptr<IntervalSet[]> delivered_messages;
    HostLookup host_lookup;

public:
    DeliverySet(HostLookup host_lookup)
        : delivered_messages(std::unique_ptr<IntervalSet[]>(
              new IntervalSet[host_lookup.get_host_count()])),
          host_lookup(host_lookup) {}

    bool is_delivered(TransportMessage message) {
        std::unique_lock<std::mutex> lck(this->lock);
        uint8_t host_id = static_cast<uint8_t>(
            this->host_lookup.get_host_id_by_ip(message.address) - 1);

        return this->delivered_messages[host_id].contains(message.get_id());
    }

    void deliver(TransportMessage message) {
        std::unique_lock<std::mutex> lck(this->lock);
        uint8_t host_id = static_cast<uint8_t>(
            this->host_lookup.get_host_id_by_ip(message.address) - 1);
        this->delivered_messages[host_id].insert(message.get_id());
    }
};
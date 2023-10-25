#pragma once

#include "host_lookup.hpp"
#include "transport_message.hpp"
#include <unordered_map>
#include <mutex>
#include <unordered_set>
#include "interval_set.hpp"

class AckSet
{
private:
    std::mutex lock;
    std::unique_ptr<IntervalSet[]> acked_messages;
    HostLookup host_lookup;

public:
    AckSet(HostLookup host_lookup) : acked_messages(std::unique_ptr<IntervalSet[]>(new IntervalSet[host_lookup.get_host_count()])), host_lookup(host_lookup) {}

    bool is_acked(TransportMessage message)
    {
        uint8_t host_id = static_cast<uint8_t>(this->host_lookup.get_host_id_by_ip(message.address) - 1);
        this->lock.lock();
        bool found = this->acked_messages.get()[host_id].contains(message.get_id());
        this->lock.unlock();

        return found;
    }

    void ack(TransportMessage message)
    {
        uint8_t host_id = static_cast<uint8_t>(this->host_lookup.get_host_id_by_ip(message.address) - 1);

        this->lock.lock();
        this->acked_messages.get()[host_id].insert(message.get_id());
        this->lock.unlock();
    }
};
#pragma once

#include "host_lookup.hpp"
#include "transport_message.hpp"
#include <unordered_map>
#include <mutex>
#include <unordered_set>

class AckSetMap
{
private:
    std::mutex lock;
    std::unordered_map<uint32_t, std::unordered_set<uint8_t>> acked_messages;
    HostLookup host_lookup;

public:
    AckSetMap(HostLookup host_lookup) : host_lookup(host_lookup) {}

    bool is_acked(TransportMessage message)
    {
        uint8_t host_id = this->host_lookup.get_host_id_by_ip(message.address);
        this->lock.lock();
        auto iterator = this->acked_messages.find(message.get_id());
        auto set =
            iterator == this->acked_messages.cend() ? std::unordered_set<uint8_t>() : iterator->second;
        bool found = !(set.find(host_id) == set.cend());
        this->lock.unlock();

        return found;
    }

    void ack(TransportMessage message)
    {
        uint8_t host_id = this->host_lookup.get_host_id_by_ip(message.address);

        this->lock.lock();
        if (this->acked_messages.find(message.get_id()) == this->acked_messages.cend())
        {
            std::unordered_set<uint8_t> set;
            set.insert(host_id);
            this->acked_messages.emplace(message.get_id(), set);
            this->lock.unlock();

            return;
        }
        this->acked_messages.find(message.get_id())->second.insert(host_id);
        this->lock.unlock();
    }
};
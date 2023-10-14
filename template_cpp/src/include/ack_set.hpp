#pragma once

#include "host_lookup.hpp"
#include "transport_message.hpp"
#include <map>
#include <mutex>
#include <set>

class AckSet {
private:
    std::mutex lock;
    std::map<uint64_t, std::set<uint64_t>> acked_messages;
    HostLookup host_lookup;

public:
    AckSet(HostLookup host_lookup) : host_lookup(host_lookup) {}

    bool is_acked(TransportMessage message) {
        std::cout << "Checking ack " << message.get_id() << std::endl;
        uint64_t host_id = this->host_lookup.get_host_id_by_ip(message.address);
        std::cout << "Andrija10: " << host_id << std::endl;
        this->lock.lock();
        auto iterator = this->acked_messages.find(message.get_id());
        auto set =
            iterator == this->acked_messages.cend() ? std::set<uint64_t>() : iterator->second;
        bool found = !(set.find(host_id) == set.cend());
        this->lock.unlock();
        std::cout << "Finished checking ack " << message.get_id() << std::endl;

        return found;
    }

    void ack(TransportMessage message) {
        std::cout << "Acking message " << message.get_id() << std::endl;
        uint64_t host_id = this->host_lookup.get_host_id_by_ip(message.address);

        this->lock.lock();
        if (this->acked_messages.find(message.get_id()) == this->acked_messages.cend()) {
            std::set<uint64_t> set;
            set.insert(host_id);
            this->acked_messages.emplace(message.get_id(), set);
            this->lock.unlock();

            return;
        }
        this->acked_messages.find(message.get_id())->second.insert(host_id);
        this->lock.unlock();
        std::cout << "Finished acking " << message.get_id() << std::endl;
    }
};
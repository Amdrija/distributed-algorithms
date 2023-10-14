#pragma once

#include "host_lookup.hpp"
#include "transport_message.hpp"
#include <map>
#include <mutex>
#include <set>

class DeliverySet {
private:
    std::map<uint64_t, std::set<uint64_t>> delivered_messages;
    HostLookup host_lookup;

public:
    DeliverySet(HostLookup host_lookup) : host_lookup(host_lookup) {}

    bool is_delivered(TransportMessage message) {
        std::cout << "Checking delivered " << message.get_id() << std::endl;
        uint64_t host_id = this->host_lookup.get_host_id_by_ip(message.address);
        std::cout << "Andrija11: " << host_id << std::endl;
        auto iterator = this->delivered_messages.find(message.get_id());
        auto set =
            iterator == this->delivered_messages.cend() ? std::set<uint64_t>() : iterator->second;
        bool found = !(set.find(host_id) == set.cend());
        std::cout << "Finished checking delivery " << message.get_id() << std::endl;

        return found;
    }

    void deliver(TransportMessage message) {
        std::cout << "Delivering message " << message.get_id() << std::endl;
        uint64_t host_id = this->host_lookup.get_host_id_by_ip(message.address);
        if (this->delivered_messages.find(message.get_id()) == this->delivered_messages.cend()) {
            std::set<uint64_t> set;
            set.insert(host_id);
            this->delivered_messages.emplace(message.get_id(), set);
            std::cout << "Finished delivering " << message.get_id() << std::endl;
            return;
        }
        this->delivered_messages.find(message.get_id())->second.insert(host_id);
        std::cout << "Finished delivering " << message.get_id() << std::endl;
    }
};
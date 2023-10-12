#pragma once

#include "host_lookup.hpp"
#include "network_config.hpp"

class Config {
public:
    NetworkConfig net_config;
    HostLookup hosts;
    uint64_t id;

    Config(const std::string &config_file, const std::string &hosts_file, uint64_t id) : net_config(config_file), hosts(hosts_file) {
        std::cout << "Sender id: " << net_config.get_receiver_id() << " messages: " << net_config.get_message_count() << std::endl;
        this->id = id;
    }
};
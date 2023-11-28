#pragma once

#include "address.hpp"
#include "string_helpers.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <unordered_map>
#include <vector>

class HostLookup {
private:
    std::unordered_map<uint32_t, std::unordered_map<uint16_t, uint8_t>>
        address_to_host;
    std::unordered_map<uint8_t, Address> host_to_address;

public:
    HostLookup(std::string file_name) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        std::string line;
        std::ifstream file(file_name);

        while (getline(file, line)) {

            // splitted[0] is the id of the process
            // splitted[1] is the ipv4 address of the process
            // splitted[2] is the port
            std::vector<std::string> split_vec =
                StringHelpers::split(line, " ");

            uint8_t id = static_cast<uint8_t>(std::stoul(split_vec[0]));
            std::string ip =
                split_vec[1] == "localhost" ? "127.0.0.1" : split_vec[1];
            Address address =
                Address(ip, static_cast<uint16_t>(std::stoul(split_vec[2])));
            host_to_address.emplace(id, address);
            address_to_host[address.ip][address.port] = id;
        }
    }

    // In a system of n identifiers, the processes have ids from 1 to n
    // this function can therefore return 0 if it cannot find a process
    // with specified ip.
    uint8_t get_host_id_by_ip(Address address) const {
        auto inner_map = address_to_host.find(address.ip);
        if (inner_map == address_to_host.end()) {
            return 0;
        }

        auto id = inner_map->second.find(address.port);
        if (id == inner_map->second.end()) {
            return 0;
        }

        return id->second;
    }

    Address get_address_by_host_id(uint8_t id) const {
        auto pair = host_to_address.find(id);

        return pair == host_to_address.end() ? Address(0, 0) : pair->second;
    }

    std::vector<uint8_t> get_hosts() {
        std::vector<uint8_t> hosts;
        for (auto pair : this->host_to_address) {
            hosts.push_back(pair.first);
        }

        return hosts;
    }

    std::vector<uint8_t> get_random_hosts() {
        std::vector<uint8_t> hosts = this->get_hosts();
        for (uint8_t i = 0; i <= hosts.size() - 2; i++) {
            uint8_t j =
                static_cast<uint8_t>(i + std::rand() % (hosts.size() - i));

            auto temp = hosts[i];
            hosts[i] = hosts[j];
            hosts[j] = temp;
        }

        return hosts;
    }

    uint8_t get_host_count() {
        return static_cast<uint8_t>(this->host_to_address.size());
    }

private:
    unsigned int convert_ipv4_to_unsigned_int(const std::string &ipv4) const {
        if (ipv4.compare("localhost")) {
            return inet_addr("127.0.0.1");
        }

        return inet_addr(ipv4.c_str());
    }
};
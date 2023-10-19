#pragma once

#include "address.hpp"
#include "string_helpers.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <map>

class HostLookup {
private:
    std::map<uint32_t, std::map<uint16_t, uint64_t>> address_to_host;
    std::map<uint64_t, Address> host_to_address;

public:
    HostLookup(std::string file_name) {
        std::string line;
        std::ifstream file(file_name);

        while (getline(file, line)) {

            // splitted[0] is the id of the process
            // splitted[1] is the ipv4 address of the process
            // splitted[2] is the port
            std::vector<std::string> split_vec = StringHelpers::split(line, " ");

            uint64_t id = std::stoul(split_vec[0]);
            std::string ip = split_vec[1] == "localhost" ? "127.0.0.1" : split_vec[1];
            Address address = Address(ip, static_cast<uint16_t>(std::stoul(split_vec[2])));
            host_to_address.emplace(id, address);
            address_to_host[address.ip][address.port] = id;
        }
    }

    // In a system of n identifiers, the processes have ids from 1 to n
    // this function can therefore return 0 if it cannot find a process
    // with specified ip.
    uint64_t get_host_id_by_ip(Address address) const {
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

    Address get_address_by_host_id(uint64_t id) const {
        auto pair = host_to_address.find(id);

        return pair == host_to_address.end() ? Address(0, 0) : pair->second;
    }

private:
    unsigned int convert_ipv4_to_unsigned_int(const std::string &ipv4) const {
        if (ipv4.compare("localhost")) {
            return inet_addr("127.0.0.1");
        }

        return inet_addr(ipv4.c_str());
    }
};
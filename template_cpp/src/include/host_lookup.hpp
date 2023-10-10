#pragma once

#include "string_helpers.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <map>

class HostLookup {
private:
    std::map<uint32_t, std::map<uint16_t, uint64_t>> map;

public:
    HostLookup(std::string file_name) {
        std::string line;
        std::ifstream file(file_name);

        while (getline(file, line)) {

            // splitted[0] is the id of the process
            // splitted[1] is the ipv4 address of the process
            // splitted[2] is the port
            std::vector<std::string> split_vec = StringHelpers::split(line, " ");

            uint32_t ipv4 = convert_ipv4_to_unsigned_int(split_vec[1]);
            uint16_t port = std::stoi(split_vec[2]);
            uint64_t id = std::stoul(split_vec[0]);
            map[ipv4][port] = id;
        }
    }

    // In a system of n identifiers, the processes have ids from 1 to n
    // this function can therefore return 0 if it cannot find a process
    // with specified ip.
    uint16_t get_host_id_by_ip(uint32_t ipv4, uint16_t port) {
        auto inner_map = map.find(ipv4);
        if (inner_map == map.end()) {
            return 0;
        }

        auto id = inner_map->second.find(port);
        if (id == inner_map->second.end()) {
            return 0;
        }

        return id->second;
    }

private:
    unsigned int convert_ipv4_to_unsigned_int(const std::string &ipv4) {
        if (ipv4 == "localhost") {
            return inet_addr("127.0.0.1");
        }

        return inet_addr(ipv4.c_str());
    }
};
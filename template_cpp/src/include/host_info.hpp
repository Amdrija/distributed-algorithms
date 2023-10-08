#pragma once

#include "string_helpers.hpp"
#include <arpa/inet.h>
#include <fstream>
#include <map>

class HostInfo {
private:
    std::map<u_int32_t, std::map<u_int16_t, u_int16_t>> map;

public:
    HostInfo(const std::string &file_name) {
        std::string line;
        std::ifstream file(file_name);

        while (getline(file, line)) {

            // splitted[0] is the id of the process
            // splitted[1] is the ipv4 address of the process
            // splitted[2] is the port
            std::vector<std::string> split_vec = split(line, " ");
            map[convert_ipv4_to_unsigned_int(split_vec[1])][std::stoi(split_vec[2])] = std::stoi(split_vec[0]);
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
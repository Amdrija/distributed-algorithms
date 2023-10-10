#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <cstring>
#include <string>

class Address {
public:
    const uint32_t ip;
    const uint16_t port;

    Address(sockaddr_in ipv4) : Address((ntohl(ipv4.sin_addr.s_addr)), ntohs(ipv4.sin_port)) {}

    Address(const std::string &ip, uint16_t port) : Address((ntohl(inet_addr(ip.c_str()))), port) {}

    Address(uint32_t ip, uint16_t port) : ip(ip), port(port) {}

    std::string to_string() const {
        char source_buffer[16];
        in_addr net_ip;
        net_ip.s_addr = htonl(this->ip);
        std::string result = std::string(inet_ntoa(net_ip));
        result.append(":");
        result.append(std::to_string(static_cast<int>(this->port)));

        return result;
    }

    sockaddr_in to_sockaddr() const {
        sockaddr_in address;
        memset(reinterpret_cast<void *>(&address), 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(this->ip);
        address.sin_port = htons(port);

        return address;
    }
};
#pragma once

#include "transport_message.hpp"
#include <arpa/inet.h>
#include <cstdint>

class Message {
public:
    uint32_t ip;
    uint16_t port;
    TransportMessage transport_message;

    Message(const std::string &ip, uint16_t port, TransportMessage transport_message) : Message(ntohl(inet_addr(ip.c_str())), port, transport_message) {
    }

    Message(uint32_t ip, uint16_t port, TransportMessage transport_message) : transport_message(transport_message) {
        this->ip = ip;
        this->port = port;
    }

    std::string get_readable_ip() {
        char source_buffer[16];
        in_addr net_ip;
        net_ip.s_addr = htonl(this->ip);
        std::string result = std::string(inet_ntoa(net_ip));
        result.append(":");
        result.append(std::to_string(static_cast<int>(this->port)));

        return result;
    }
};
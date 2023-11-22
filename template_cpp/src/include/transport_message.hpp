#pragma once

#include "address.hpp"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

class TransportMessage {
private:
    static std::atomic_uint32_t next_id;
    uint32_t id;
    std::shared_ptr<char[]> payload;

    TransportMessage(Address address, const uint32_t id);

public:
    const Address address;
    const uint64_t length;
    bool is_ack;

    // constructor for creating transport message from message
    TransportMessage(Address address, std::shared_ptr<char[]> payload,
                     const uint64_t length);

    // constructor for deserializing a message
    TransportMessage(Address address, const char *bytes, uint64_t length);

    std::unique_ptr<char[]> serialize(uint64_t &serialized_length);

    std::shared_ptr<char[]> get_payload();

    uint32_t get_id();

    static TransportMessage create_ack(TransportMessage &message);
};
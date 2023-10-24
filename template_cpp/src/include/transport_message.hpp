#pragma once

#include "address.hpp"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>

class TransportMessage
{
private:
    uint32_t id;
    std::shared_ptr<char[]> payload;

public:
    const Address address;
    const uint64_t length;
    bool is_ack;

    // constructor for creating transport message from message
    TransportMessage(Address address, const uint32_t id, std::shared_ptr<char[]> payload,
                     const uint64_t length, bool is_ack = false)
        : id(id), payload(payload), address(address), length(length), is_ack(is_ack)
    {
        // std::memcpy(payload.get(), bytes, length);
    }

    // constructor for deserializing a message
    TransportMessage(Address address, const char *bytes, uint64_t length)
        : id(0), payload(new char[length - sizeof(id) - sizeof(is_ack)]), address(address),
          length(length - sizeof(id) - sizeof(is_ack))
    {
        std::memcpy(&this->id, bytes, sizeof(this->id));
        std::memcpy(&this->is_ack, bytes + sizeof(this->id), sizeof(this->is_ack));
        std::memcpy(payload.get(), bytes + sizeof(this->id) + sizeof(this->is_ack),
                    length - sizeof(this->id) - sizeof(this->is_ack));
    }

    std::unique_ptr<char[]> serialize(uint64_t &serialized_length)
    {
        serialized_length = this->length + sizeof(this->id) + sizeof(this->is_ack);
        std::unique_ptr<char[]> bytes = std::unique_ptr<char[]>(new char[serialized_length]);
        std::memcpy(bytes.get(), &this->id, sizeof(this->id));
        std::memcpy(bytes.get() + sizeof(this->id), &this->is_ack, sizeof(this->is_ack));
        std::memcpy(bytes.get() + sizeof(this->id) + sizeof(this->is_ack), this->payload.get(),
                    this->length);

        return std::move(bytes);
    }

    std::shared_ptr<char[]> get_payload() { return this->payload; }

    uint32_t get_id() { return this->id; }
};
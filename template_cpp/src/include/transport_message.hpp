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
    uint64_t id;
    std::shared_ptr<char[]> payload;

public:
    const Address address;
    const uint64_t length;
    bool is_ack;

    // constructor for creating transport message from message
    TransportMessage(Address address, const uint64_t id, std::shared_ptr<char[]> payload,
                     const uint64_t length, bool is_ack = false)
        : id(id), payload(payload), address(address), length(length), is_ack(is_ack) {
        // std::memcpy(payload.get(), bytes, length);
    }

    // constructor for deserializing a message
    TransportMessage(Address address, const char *bytes, uint64_t length)
        : id(0), payload(new char[length - sizeof(uint64_t) - sizeof(bool)]), address(address),
          length(length - sizeof(uint64_t) - sizeof(bool)) {
        std::memcpy(&this->id, bytes, sizeof(uint64_t));
        std::memcpy(&this->is_ack, bytes + sizeof(uint64_t), sizeof(bool));
        std::memcpy(payload.get(), bytes + sizeof(uint64_t) + sizeof(bool),
                    length - sizeof(uint64_t) - sizeof(bool));
    }

    std::unique_ptr<char[]> serialize(uint64_t &serialized_length) {
        serialized_length = this->length + sizeof(uint64_t) + sizeof(bool);
        std::unique_ptr<char[]> bytes = std::unique_ptr<char[]>(new char[serialized_length]);
        std::memcpy(bytes.get(), &this->id, sizeof(uint64_t));
        std::memcpy(bytes.get() + sizeof(uint64_t), &this->is_ack, sizeof(bool));
        std::memcpy(bytes.get() + sizeof(uint64_t) + sizeof(bool), this->payload.get(),
                    this->length);

        return std::move(bytes);
    }

    std::shared_ptr<char[]> get_payload() { return this->payload; }

    uint64_t get_id() { return this->id; }
};
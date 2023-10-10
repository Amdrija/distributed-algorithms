#pragma once

#include "address.hpp"
#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

class TransportMessage {
private:
    uint64_t id;
    std::unique_ptr<char[]> payload;

public:
    const Address address;
    const uint64_t length;

    TransportMessage(Address address, const std::string &m) : TransportMessage(address, 0, m.c_str(), m.length()) {
    }

    TransportMessage(Address address, uint64_t id, const char *bytes, uint64_t length) : id(id), payload(new char[length]), address(address), length(length) {
        std::memcpy(payload.get(), bytes, length);
    }

    TransportMessage(Address address, const char *bytes, uint64_t length) : id(0), payload(new char[length - 8]), address(address), length(length - 8) {
        std::memcpy(&this->id, bytes, 8);
        std::memcpy(payload.get(), bytes + 8, length - 8);
    }

    TransportMessage(TransportMessage &&m) : id(m.id), payload(std::move(m.payload)), address(m.address), length(m.length) {}

    std::unique_ptr<char[]> serialize(uint64_t &serialized_length) {
        serialized_length = this->length + 8;
        std::unique_ptr<char[]> bytes = std::unique_ptr<char[]>(new char[serialized_length]);
        std::memcpy(bytes.get(), &this->id, 8);
        std::memcpy(bytes.get() + 8, this->payload.get(), this->length);

        return std::move(bytes);
    }

    std::unique_ptr<char[]> get_payload() && {
        return std::move(this->payload);
    }
};
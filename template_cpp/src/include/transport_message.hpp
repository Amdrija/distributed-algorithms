#pragma once

#include <atomic>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>

class TransportMessage {
public:
    uint64_t id;
    uint64_t length;
    std::shared_ptr<char[]> payload;

    TransportMessage(const std::string &m) : TransportMessage(m.c_str(), m.length()) {
    }

    TransportMessage(const char *bytes, uint64_t length) : payload(new char[length]) {
        this->id = UINT64_MAX;
        this->length = length;
        std::memcpy(payload.get(), bytes, length);
    }

    std::unique_ptr<char[]> serialize(uint64_t &serialized_length) {
        serialized_length = this->length + 8;
        std::unique_ptr<char[]> bytes = std::unique_ptr<char[]>(new char[serialized_length]);
        std::memcpy(bytes.get(), &this->id, 8);
        std::memcpy(bytes.get() + 8, this->payload.get(), this->length);

        return std::move(bytes);
    }

    static TransportMessage deserialize(const char *bytes, uint64_t length) {
        TransportMessage m(bytes + 8, length - 8);
        std::memcpy(&m.id, bytes, 8);

        return m;
    }

private:
    // static std::atomic<uint64_t> counter;
};
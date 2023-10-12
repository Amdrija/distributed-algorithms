#pragma once

#include "transport_message.hpp"
#include <atomic>

class Message {
private:
    static std::atomic_uint64_t next_id;

protected:
    Message();

public:
    const uint64_t id;
    virtual std::shared_ptr<char[]> serialize(uint64_t &length) = 0;
};

class EmptyMessage : public Message {
    virtual std::shared_ptr<char[]> serialize(uint64_t &length) {
        length = 0;
        std::shared_ptr<char[]> p(new char[length]);

        return p;
    }
};

class StringMessage : public Message {
private:
    std::string message;

public:
    StringMessage(std::string message) : Message(), message(message) {}

    virtual std::shared_ptr<char[]> serialize(uint64_t &length) {
        length = this->message.length();
        std::shared_ptr<char[]> p(new char[length]);
        std::memcpy(p.get(), message.c_str(), length);

        return p;
    }
};
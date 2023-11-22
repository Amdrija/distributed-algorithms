#pragma once

#include "transport_message.hpp"

enum MessageType { Empty, String, Ack };

class Message {
protected:
    MessageType type;
    Message(const MessageType type) : type(type) {}

public:
    virtual std::unique_ptr<char[]> serialize(uint64_t &length) = 0;

    MessageType get_type() { return this->type; }
};

class EmptyMessage : public Message {
public:
    EmptyMessage() : Message(MessageType::Empty) {}

    EmptyMessage(std::shared_ptr<char[]> payload, const uint64_t length)
        : Message(MessageType::Empty) {
        // we can ignore the payload as this message only contains it's type.
    }

    virtual std::unique_ptr<char[]> serialize(uint64_t &length) {
        length = sizeof(MessageType);
        std::unique_ptr<char[]> payload(new char[length]);
        std::memcpy(payload.get(), &this->type, length);

        return payload;
    }
};

class StringMessage : public Message {
private:
    std::string message;

public:
    StringMessage(const std::string message)
        : Message(MessageType::String), message(message) {}

    StringMessage(std::shared_ptr<char[]> payload, const uint64_t length)
        : Message(MessageType::String) {
        // first we skip the type, only copy the payload;
        this->message = std::string(payload.get() + sizeof(MessageType),
                                    length - sizeof(MessageType));
    }

    virtual std::unique_ptr<char[]> serialize(uint64_t &length) {
        length = this->message.length() + sizeof(MessageType);
        std::unique_ptr<char[]> payload(new char[length]);
        std::memcpy(payload.get(), &this->type, sizeof(MessageType));
        std::memcpy(payload.get() + sizeof(MessageType), this->message.c_str(),
                    length - sizeof(MessageType));

        return payload;
    }

    std::string get_message() { return this->message; }
};
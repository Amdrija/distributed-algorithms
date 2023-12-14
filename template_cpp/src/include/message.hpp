#pragma once

#include "transport_message.hpp"
#include <unordered_set>

enum MessageType { Empty, String, Broadcast, Proposal };

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
        length = this->message.length() + 1 + sizeof(MessageType);
        std::unique_ptr<char[]> payload(new char[length]);
        std::memcpy(payload.get(), &this->type, sizeof(MessageType));
        std::memcpy(payload.get() + sizeof(MessageType), this->message.c_str(),
                    length - sizeof(MessageType));

        return payload;
    }

    std::string get_message() { return this->message; }
};

class BroadcastMessage : public Message {
private:
    uint32_t sequence_number;
    uint8_t source;
    uint64_t message_length;
    std::unique_ptr<char[]> message_payload;

public:
    BroadcastMessage(Message &m, uint32_t sequence_number, uint8_t source)
        : Message(MessageType::Broadcast), sequence_number(sequence_number),
          source(source) {
        this->message_payload = m.serialize(this->message_length);
    }

    BroadcastMessage(std::shared_ptr<char[]> payload, const uint64_t length)
        : Message(MessageType::Broadcast) {
        // We can skip the type
        ssize_t offset = sizeof(this->type);

        std::memcpy(&this->sequence_number, payload.get() + offset,
                    sizeof(this->sequence_number));
        offset += sizeof(this->sequence_number);

        std::memcpy(&this->source, payload.get() + offset,
                    sizeof(this->source));
        offset += sizeof(this->source);

        std::memcpy(&this->message_length, payload.get() + offset,
                    sizeof(this->message_length));
        offset += sizeof(this->message_length);

        this->message_payload =
            std::unique_ptr<char[]>(new char[this->message_length]);
        std::memcpy(this->message_payload.get(), payload.get() + offset,
                    this->message_length);
    }

    virtual std::unique_ptr<char[]> serialize(uint64_t &length) {
        length = sizeof(this->type) + sizeof(this->sequence_number) +
                 sizeof(this->source) + sizeof(this->message_length) +
                 this->message_length;

        std::unique_ptr<char[]> payload(new char[length]);
        ssize_t offset = 0;

        std::memcpy(payload.get() + offset, &this->type, sizeof(this->type));
        offset += sizeof(this->type);

        std::memcpy(payload.get() + offset, &this->sequence_number,
                    sizeof(this->sequence_number));
        offset += sizeof(this->sequence_number);

        std::memcpy(payload.get() + offset, &this->source,
                    sizeof(this->source));
        offset += sizeof(this->source);

        std::memcpy(payload.get() + offset, &this->message_length,
                    sizeof(this->message_length));
        offset += sizeof(this->message_length);

        std::memcpy(payload.get() + offset, this->message_payload.get(),
                    this->message_length);

        return payload;
    }

    std::unique_ptr<Message> get_message() && {
        MessageType message_type;
        std::memcpy(&message_type, this->message_payload.get(),
                    sizeof(message_type));

        switch (message_type) {
        case MessageType::Empty:
            return std::unique_ptr<EmptyMessage>(new EmptyMessage(
                std::move(this->message_payload), this->message_length));
        case MessageType::String:
            return std::unique_ptr<StringMessage>(new StringMessage(
                std::move(this->message_payload), this->message_length));
        case MessageType::Broadcast:
            return std::unique_ptr<BroadcastMessage>(new BroadcastMessage(
                std::move(this->message_payload), this->message_length));
        default:
            throw "Unknown message type: " +
                std::to_string(static_cast<int>(type));
            break;
        }

        return std::unique_ptr<EmptyMessage>(new EmptyMessage(
            std::move(this->message_payload), this->message_length));
    }

    uint32_t get_sequence_number() { return this->sequence_number; }
    uint8_t get_source() { return static_cast<uint8_t>(this->source); }
    uint64_t get_message_length() { return this->message_length; }
};

typedef uint64_t propose_value;

class ProposeMessage : public Message {
public:
    uint64_t round;
    uint64_t proposal_number;
    std::unordered_set<propose_value> proposed_values;
    ProposeMessage(uint64_t round, uint64_t proposal_number,
                   std::unordered_set<propose_value> proposed_value)
        : Message(MessageType::Proposal), round(round),
          proposal_number(proposal_number), proposed_values(proposed_value) {}

    ProposeMessage(std::shared_ptr<char[]> payload, const uint64_t length)
        : Message(MessageType::Proposal) {
        size_t offset = sizeof(this->type);
        std::memcpy(&this->round, payload.get() + offset, sizeof(this->round));
        offset += sizeof(this->round);

        std::memcpy(&this->proposal_number, payload.get() + offset,
                    sizeof(this->proposal_number));
        offset += sizeof(this->proposal_number);

        size_t set_size;
        std::memcpy(&set_size, payload.get() + offset, sizeof(set_size));
        offset += sizeof(set_size);

        for (size_t i = 0; i < set_size; i++) {
            propose_value value;
            std::memcpy(&value, payload.get() + offset, sizeof(value));
            offset += sizeof(value);

            this->proposed_values.emplace(value);
        }
    }

    virtual std::unique_ptr<char[]> serialize(uint64_t &length) {
        size_t set_size = this->proposed_values.size();
        length = sizeof(type) + sizeof(round) + sizeof(proposal_number) +
                 sizeof(set_size) + sizeof(propose_value) * set_size;

        std::unique_ptr<char[]> payload(new char[length]);

        size_t offset = 0;
        std::memcpy(payload.get() + offset, &this->type, sizeof(this->type));
        offset += sizeof(this->type);

        std::memcpy(payload.get() + offset, &this->round, sizeof(this->round));
        offset += sizeof(this->round);

        std::memcpy(payload.get() + offset, &this->proposal_number,
                    sizeof(this->proposal_number));
        offset += sizeof(this->proposal_number);

        std::memcpy(payload.get() + offset, &set_size, sizeof(set_size));
        offset += sizeof(set_size);

        for (auto value : this->proposed_values) {
            std::memcpy(payload.get() + offset, &value, sizeof(value));
            offset += sizeof(value);
        }

        return payload;
    }
};
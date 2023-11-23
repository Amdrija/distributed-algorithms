#pragma once

#include "host_lookup.hpp"
#include "transport_message.hpp"
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>

#define MAX_MESSAGE_COUNT 8

// TODO: Implement this with thread safety
// Otherwise the receiving thread and the sending thread
// could modify the same buffer at the same time
class SendBuffer {
private:
    uint64_t capacity;
    std::unordered_map<uint8_t, uint64_t> sizes;
    std::unordered_map<uint8_t, std::unique_ptr<char[]>> buffers;
    std::unordered_map<uint8_t, uint8_t> message_counts;
    HostLookup host_lookup;
    std::mutex lock;

public:
    SendBuffer(HostLookup host_lookup, uint64_t initial_capacity)
        : capacity(initial_capacity), host_lookup(host_lookup) {
        for (auto host : host_lookup.get_hosts()) {
            this->sizes.emplace(host, 0);
            this->buffers.emplace(
                host, std::unique_ptr<char[]>(new char[initial_capacity]));
            this->message_counts.emplace(host, 0);
        }
    }

    std::unique_ptr<char[]> add_message(TransportMessage message,
                                        uint64_t &payload_length) {

        uint64_t serialized_length = 0;
        std::unique_ptr<char[]> payload = message.serialize(serialized_length);
        uint8_t host_id = host_lookup.get_host_id_by_ip(message.address);

        this->lock.lock();
        if (this->message_counts[host_id] == MAX_MESSAGE_COUNT ||
            serialized_length + this->sizes[host_id] +
                    sizeof(serialized_length) >
                this->capacity) {
            std::unique_ptr<char[]> buffer_payload(
                new char[this->sizes[host_id]]);
            std::memcpy(buffer_payload.get(), this->buffers[host_id].get(),
                        this->sizes[host_id]);
            payload_length = this->sizes[host_id];

            std::memcpy(this->buffers[host_id].get(), &serialized_length,
                        sizeof(serialized_length));
            std::memcpy(this->buffers[host_id].get() +
                            sizeof(serialized_length),
                        payload.get(), serialized_length);
            this->sizes[host_id] =
                sizeof(serialized_length) + serialized_length;
            this->message_counts[host_id] = 1;

            this->lock.unlock();
            return buffer_payload;
        }

        std::memcpy(this->buffers[host_id].get() + this->sizes[host_id],
                    &serialized_length, sizeof(serialized_length));
        std::memcpy(this->buffers[host_id].get() + this->sizes[host_id] +
                        sizeof(serialized_length),
                    payload.get(), serialized_length);
        this->sizes[host_id] += sizeof(serialized_length) + serialized_length;
        this->message_counts[host_id]++;

        this->lock.unlock();
        payload_length = 0;
        return std::unique_ptr<char[]>(new char[0]);
    }

    std::vector<TransportMessage> deserialize(Address address, char *buffer,
                                              ssize_t received_length) {
        std::vector<TransportMessage> messages;
        ssize_t start = 0;
        uint64_t message_length;
        while (start < received_length) {
            std::memcpy(&message_length, buffer + start,
                        sizeof(message_length));
            messages.push_back(TransportMessage(
                address, buffer + start + sizeof(message_length),
                message_length));
            start += sizeof(message_length) + message_length;
        }

        return messages;
    }
};
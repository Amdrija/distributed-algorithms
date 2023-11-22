#include "transport_message.hpp"

std::atomic_uint32_t TransportMessage::next_id = 0;

TransportMessage::TransportMessage(Address address, const uint32_t id)
    : id(id), payload(new char[0]), address(address), length(0), is_ack(true) {
    // std::memcpy(payload.get(), bytes, length);
}

TransportMessage::TransportMessage(Address address,
                                   std::shared_ptr<char[]> payload,
                                   const uint64_t length)
    : id(++TransportMessage::next_id), payload(payload), address(address),
      length(length), is_ack(false) {
    // std::memcpy(payload.get(), bytes, length);
}

TransportMessage::TransportMessage(Address address, const char *bytes,
                                   uint64_t length)
    : id(0), payload(new char[length - sizeof(id) - sizeof(is_ack)]),
      address(address), length(length - sizeof(id) - sizeof(is_ack)) {
    std::memcpy(&this->id, bytes, sizeof(this->id));
    std::memcpy(&this->is_ack, bytes + sizeof(this->id), sizeof(this->is_ack));
    std::memcpy(payload.get(), bytes + sizeof(this->id) + sizeof(this->is_ack),
                length - sizeof(this->id) - sizeof(this->is_ack));
}

std::unique_ptr<char[]>
TransportMessage::serialize(uint64_t &serialized_length) {
    serialized_length = this->length + sizeof(this->id) + sizeof(this->is_ack);
    std::unique_ptr<char[]> bytes =
        std::unique_ptr<char[]>(new char[serialized_length]);
    std::memcpy(bytes.get(), &this->id, sizeof(this->id));
    std::memcpy(bytes.get() + sizeof(this->id), &this->is_ack,
                sizeof(this->is_ack));
    std::memcpy(bytes.get() + sizeof(this->id) + sizeof(this->is_ack),
                this->payload.get(), this->length);

    return std::move(bytes);
}

std::shared_ptr<char[]> TransportMessage::get_payload() {
    return this->payload;
}

uint32_t TransportMessage::get_id() { return this->id; }

TransportMessage TransportMessage::create_ack(TransportMessage &message) {
    return TransportMessage(message.address, message.id);
}
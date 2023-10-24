#include "message.hpp"
#include <atomic>

std::atomic_uint32_t Message::next_id = 0;

Message::Message(const MessageType type) : id(++Message::next_id), type(type) {}
Message::Message(const uint32_t id, const MessageType type) : id(id), type(type) {}

uint32_t Message::get_id() { return this->id; }

MessageType Message::get_type() { return this->type; }
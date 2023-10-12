#include "message.hpp"
#include <atomic>

std::atomic_uint64_t Message::next_id = 0;
Message::Message() : id(++Message::next_id) {}
#pragma once

#include "transport_message.hpp"

class Message {
    virtual std::unique_ptr<char[]> serialize();
};
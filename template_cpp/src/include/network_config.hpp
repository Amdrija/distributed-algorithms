#pragma once

#include "string_helpers.hpp"
#include <fstream>
#include <iostream>

class NetworkConfig {
public:
    NetworkConfig(const std::string &file_name) {
        std::string line;
        std::ifstream file(file_name);

        getline(file, line);
        std::vector<std::string> args = StringHelpers::split(line, " ");
        this->message_count = std::stoul(args[0]);
        this->receiver_id = std::stoul(args[1]);
    }

    uint64_t get_receiver_id() {
        return receiver_id;
    }

    uint64_t get_message_count() {
        return message_count;
    }

private:
    uint64_t receiver_id;
    uint64_t message_count;
};
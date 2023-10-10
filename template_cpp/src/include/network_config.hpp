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
        this->sender_id = std::stoul(args[1]);
    }

    u_int64_t get_sender_id() {
        return sender_id;
    }

    u_int64_t get_message_count() {
        return message_count;
    }

private:
    u_int64_t sender_id;
    u_int64_t message_count;
};
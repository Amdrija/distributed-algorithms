#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>

class FifoConfig {
private:
    uint32_t message_count;

public:
    FifoConfig(const std::string &file_name) {
        std::ifstream file(file_name);

        file >> this->message_count;

        file.close();
    }

    uint32_t get_message_count() { return this->message_count; }
};
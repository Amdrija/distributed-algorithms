#pragma once

#include "message.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <vector>

class LatticeConfig {
public:
    uint32_t round_count;
    uint32_t set_max_size;
    uint32_t distinct_count;
    std::vector<std::unordered_set<propose_value>> proposals;

    LatticeConfig(const std::string &file_name) {
        std::ifstream file(file_name);

        file >> this->round_count;
        file >> this->set_max_size;
        file >> this->distinct_count;

        this->proposals =
            std::vector<std::unordered_set<propose_value>>(this->round_count);
        std::string trash;
        std::getline(file, trash);
        for (uint64_t i = 0; i < this->round_count; i++) {
            std::string line;
            std::getline(file, line);

            std::istringstream input(line);
            propose_value value;

            this->proposals[i] = std::unordered_set<propose_value>();
            while (input >> value) {
                proposals[i].insert(value);
            }
        }
        file.close();
    }
};
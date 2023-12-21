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
    std::ifstream file;

    LatticeConfig(const std::string &file_name) : file(file_name) {
        this->file >> this->round_count;
        this->file >> this->set_max_size;
        this->file >> this->distinct_count;

        std::string trash;
        std::getline(file, trash);
    }

    ~LatticeConfig() { this->file.close(); }

    std::unordered_set<propose_value> get_proposal() {
        std::string line;
        std::getline(file, line);

        std::istringstream input(line);
        propose_value value;

        auto proposal = std::unordered_set<propose_value>();
        while (input >> value) {
            proposal.insert(value);
        }

        return proposal;
    }
};
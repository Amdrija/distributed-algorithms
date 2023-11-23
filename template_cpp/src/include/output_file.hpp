#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>

// TODO: Make thread safe
class OutputFile {
private:
    std::ofstream file;
    std::unique_ptr<char[]> cache;
    uint64_t cache_size;
    static const uint32_t CACHE_CAPACITY = 1000000;

public:
    OutputFile(const std::string file_name)
        : file(), cache(std::unique_ptr<char[]>(new char[CACHE_CAPACITY])),
          cache_size(0) {
        this->file.open(file_name, std::ofstream::out);
    }

    ~OutputFile() {
        this->flush();
        this->close();
    }

    void write(const std::string output) {
        if (output.length() + this->cache_size > CACHE_CAPACITY) {
            this->file.write(this->cache.get(), this->cache_size);
            this->cache_size = 0;
        }

        std::memcpy(this->cache.get() + this->cache_size, output.c_str(),
                    output.length());
        this->cache_size += output.length();
    }

    void flush() {
        this->file.write(cache.get(), this->cache_size);
        this->cache_size = 0;
        this->file.flush();
    }

    void close() { this->file.close(); }
};
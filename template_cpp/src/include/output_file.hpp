#pragma once

#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

class OutputFile {
private:
    std::ofstream file;
    std::unique_ptr<char[]> cache;
    uint64_t cache_size;
    static const uint32_t CACHE_CAPACITY = 1000000;
    std::mutex lock;

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
        this->lock.lock();
        if (output.length() + this->cache_size > CACHE_CAPACITY) {
            this->file.write(this->cache.get(), this->cache_size);
            this->cache_size = 0;
        }

        std::memcpy(this->cache.get() + this->cache_size, output.c_str(),
                    output.length());
        this->cache_size += output.length();
        this->lock.unlock();
    }

    // TODO: Maybe locks aren't needed?
    void flush() {
        std::cout << "FLUSHING FILE" << std::endl;
        this->file.write(cache.get(), this->cache_size);
        this->lock.lock();
        this->cache_size = 0;
        this->file.flush();
        this->lock.unlock();
    }

    // TODO: Maybe locks aren't needed?
    void close() {
        this->lock.lock();
        std::cout << "CLOSING FILE" << std::endl;
        this->file.close();
        this->lock.unlock();
    }
};
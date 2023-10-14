#pragma once

#include <mutex>
#include <set>

template <typename T>
class ConcurrentSet {
    std::set<T> set;
    std::mutex lock;

public:
    bool contains(T item) {
        bool contains;
        this->lock.lock();
        contains = this->set.find(item) != this->set.end();
        this->lock.unlock();

        return contains;
    }

    void insert(T item) {
        this->lock.lock();
        this->set.insert(item);
        this->lock.unlock();
    }
};
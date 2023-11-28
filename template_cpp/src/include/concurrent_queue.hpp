#pragma once

#include <mutex>
#include <optional>
#include <queue>

template <typename T> class ConcurrentQueue {
private:
    std::queue<T> q;
    std::mutex lock;
    const uint32_t capacity = 20000;

public:
    void push(T item) {
        lock.lock();
        q.push(std::move(item));
        lock.unlock();
    }

    void push(std::vector<T> items) {
        lock.lock();
        for (uint32_t i = 0; i < items.size(); i++) {
            q.push(std::move(items[i]));
        }
        lock.unlock();
    }

    bool is_full() {
        lock.lock();
        bool full = this->q.size() >= capacity;
        lock.unlock();

        return full;
    }

    bool is_empty() {
        lock.lock();
        bool empty = q.empty();
        lock.unlock();

        return empty;
    }

    std::optional<T> dequeue() {
        lock.lock();
        std::optional<T> item =
            q.empty() ? std::nullopt : std::make_optional<T>(q.front());
        if (item.has_value()) {
            q.pop();
        }
        lock.unlock();

        return item;
    }
};
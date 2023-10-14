#pragma once

#include <mutex>
#include <optional>
#include <queue>

template <typename T> class ConcurrentQueue {
private:
    std::queue<T> q;
    std::mutex lock;

public:
    void push(T item) {
        lock.lock();
        q.push(std::move(item));
        lock.unlock();
    }

    bool is_empty() {
        lock.lock();
        bool empty = q.empty();
        lock.unlock();

        return empty;
    }

    std::optional<T> dequeue() {
        lock.lock();
        std::optional<T> item = q.empty() ? std::nullopt : std::make_optional<T>(q.front());
        if (item.has_value()) {
            q.pop();
        }
        lock.unlock();

        return item;
    }
};
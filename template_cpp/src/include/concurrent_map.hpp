#pragma once

#include <mutex>
#include <optional>
#include <unordered_map>

template <typename K, typename V>
class ConcurrentMap
{
    std::unordered_map<K, V> map;
    std::mutex lock;

public:
    std::optional<V &> get(K key)
    {
        this->lock.lock();
        auto iterator = this->map.find(key);
        std::optional<V &> result = iterator == this->map.cend() ? std::nullopt : iterator->second;
        this->lock.unlock();

        return result;
    }

    bool contains(K key) { return this->get(key).has_value(); }

    void insert(K key, V value)
    {
        this->lock.lock();
        this->map.emplace(std::make_pair<K, V>(key, value));
        this->lock.unlock();
    }
};
#include <mutex>
#include <queue>

template <typename T>
class ConcurrentQueue {
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
        bool empty;
        lock.lock();
        empty = q.empty();
        lock.unlock();

        return empty;
    }

    T dequeue() {
        lock.lock();
        if (q.empty()) {
            lock.unlock();
            throw 0;
        }
        T item = q.front();
        q.pop();
        lock.unlock();

        return item;
    }
};
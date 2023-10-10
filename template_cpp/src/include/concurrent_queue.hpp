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
        q.push(item);
        lock.unlock();
    }

    bool is_empty() {
        bool empty;
        lock.lock();
        empty = q.empty();
        lock.unlock();

        return empty;
    }

    void dequeue() {
        lock.lock();
        item = q.pop();
        lock.unlock();

        return item;
    }

    T first() {
        T item;
        lock.lock();
        item = q.front();
        lock.unlock();

        return item;
    }
};
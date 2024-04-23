#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

template<typename T>
class ThreadSafeQueue {
private:
    mutable std::mutex m;
    std::queue<T> queue;
    std::condition_variable data_cond;

public:
    ThreadSafeQueue() {}

    void push(T new_value) {
        std::lock_guard<std::mutex> lock(m);
        queue.push(std::move(new_value));
        data_cond.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m);
        data_cond.wait(lock, [this] { return !queue.empty(); });
        value = std::move(queue.front());
        queue.pop();
    }

    void wait_for_data() {
        std::unique_lock<std::mutex> lock(m);
        data_cond.wait(lock, [this] { return !queue.empty(); });
    }

    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m);
        if (queue.empty())
            return false;
        value = std::move(queue.front());
        queue.pop();
        return true;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m);
        return queue.empty();
    }

    unsigned long size() const {
        std::lock_guard<std::mutex> lock(m);
        return queue.size();
    }
};

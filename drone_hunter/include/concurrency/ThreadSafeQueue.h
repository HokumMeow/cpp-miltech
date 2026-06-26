#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

// Узагальнена потокобезпечна черга. Не знає нічого про DroneCommand чи
// інших споживачів — політика "брати лише останнє значення" (якщо потрібна)
// лежить на стороні викликача, а не тут.
template <typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        std::lock_guard<std::mutex> lk(mtx_);
        queue_.push(std::move(value));
        cv_.notify_one();
    }

    // Неблокуюче читання. Повертає std::nullopt, якщо черга порожня.
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lk(mtx_);
        if (queue_.empty()) return std::nullopt;
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    // Блокуюче читання з очікуванням, поки черга не отримає елемент
    // або stopFlag (atomic<bool>&) не стане true.
    std::optional<T> waitAndPop(const std::atomic<bool>& stopFlag) {
        std::unique_lock<std::mutex> lk(mtx_);
        cv_.wait(lk, [this, &stopFlag] { return !queue_.empty() || stopFlag.load(); });
        if (queue_.empty()) return std::nullopt;
        T value = std::move(queue_.front());
        queue_.pop();
        return value;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lk(mtx_);
        return queue_.empty();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

#pragma once

#include <queue>
#include <optional>

template <typename T>
class STLQueue {
public:
    STLQueue() = default;

    bool push(const T& item) {
        queue_.push(item);
        return true;
    }

    bool push(T&& item) {
        queue_.push(std::move(item));
        return true;
    }

    std::optional<T> pop() {
        if (queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    std::optional<T> top() const {
        if (queue_.empty()) {
            return std::nullopt;
        }
        return queue_.front();
    }

private:
    std::queue<T> queue_;
};
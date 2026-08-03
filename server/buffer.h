#pragma once

#include <concepts>
#include <optional>

template <typename Q, typename T>
concept Buffer = requires(Q q, const T& const_item, T&& rvalue_item, T& out_item) {
    // true if success, false if full(if bounded)
    { q.push(const_item) } -> std::convertible_to<bool>;
    { q.push(std::move(rvalue_item)) } -> std::convertible_to<bool>;

    // pop removes and returns the front item, or nullopt if empty
    { q.pop() } -> std::same_as<std::optional<T>>;

    // top peeks at the front item without removing it, or nullopt if empty
    { q.top() } -> std::same_as<std::optional<T>>;
};

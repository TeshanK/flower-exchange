#pragma once

#include "common/types.h"
#include "common/macros.h"

#include <cstddef>
#include <cstdint>
#include <vector>

class OrderBook {
public:
    // Initializes price-level arrays and active-level bitmask index.
    explicit OrderBook(std::size_t max_ticks = ORDER_BOOK_TICK_CAPACITY);

    // Hot-path insertion variant when tick validity is guaranteed by caller.
    inline void add_order_known_valid(PriceTick tick, Order* order) {
        if (UNLIKELY(!order)) {
            return;
        }

        order->next = nullptr;
        if (!price_levels_[tick]) {
            price_levels_[tick] = order;
            price_level_tails_[tick] = order;

            const std::size_t word = static_cast<std::size_t>(tick / 64);
            const uint64_t bit = (1ULL << (tick % 64));
            mask_[word] |= bit;

            if (best_first_ < 0 || static_cast<int64_t>(tick) < best_first_) {
                best_first_ = static_cast<int64_t>(tick);
            }
            if (best_last_ < 0 || static_cast<int64_t>(tick) > best_last_) {
                best_last_ = static_cast<int64_t>(tick);
            }
            return;
        }

        price_level_tails_[tick]->next = order;
        price_level_tails_[tick] = order;
    }
    // Hot-path pop variant when tick validity is guaranteed by caller.
    inline Order* pop_order_known_valid(PriceTick tick) {
        Order* head = price_levels_[tick];
        if (UNLIKELY(!head)) {
            return nullptr;
        }

        Order* next = head->next;
        price_levels_[tick] = next;
        head->next = nullptr;

        if (!next) {
            price_level_tails_[tick] = nullptr;
            const std::size_t word = static_cast<std::size_t>(tick / 64);
            const uint64_t bit = (1ULL << (tick % 64));
            mask_[word] &= ~bit;

            if (best_first_ == static_cast<int64_t>(tick)) {
                best_first_ = find_next_from(tick);
            }
            if (best_last_ == static_cast<int64_t>(tick)) {
                best_last_ = find_prev_from(tick);
            }
        }
        return head;
    }

    // Returns best (lowest) active level or -1 if empty.
    int64_t find_first() const;
    // Returns best (highest) active level or -1 if empty.
    int64_t find_last() const;

private:
    // Finds next active tick strictly above `tick`.
    int64_t find_next_from(PriceTick tick) const;
    // Finds previous active tick strictly below `tick`.
    int64_t find_prev_from(PriceTick tick) const;

    std::vector<Order*> price_levels_;
    std::vector<Order*> price_level_tails_;
    std::vector<uint64_t> mask_;
    int64_t best_first_;
    int64_t best_last_;
};

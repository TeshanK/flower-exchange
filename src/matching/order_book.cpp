#include "matching/order_book.h"
#include "common/macros.h"

#include <algorithm>

OrderBook::OrderBook(std::size_t max_ticks)
        : price_levels_(max_ticks, nullptr),
            price_level_tails_(max_ticks, nullptr),
    mask_((max_ticks + 63) / 64, 0ULL),
    best_first_(-1),
    best_last_(-1) {}

int64_t OrderBook::find_first() const {
    return best_first_;
}

int64_t OrderBook::find_last() const {
    return best_last_;
}

int64_t OrderBook::find_next_from(PriceTick tick) const {
    const std::size_t words = mask_.size();
    auto word = static_cast<std::size_t>(tick / 64);
    const auto bit_in_word = static_cast<std::size_t>(tick % 64);

    if (UNLIKELY(word >= words)) {
        return -1;
    }

    uint64_t bits = mask_[word];
    if (bit_in_word < 63) {
        const uint64_t keep_from_next = ~((1ULL << (bit_in_word + 1)) - 1ULL);
        bits &= keep_from_next;
    } else {
        bits = 0;
    }

    if (bits != 0) {
        return static_cast<int64_t>(word * 64 + __builtin_ctzll(bits));
    }

    const auto it = std::find_if(mask_.begin() + static_cast<std::ptrdiff_t>(word + 1),
                                 mask_.end(),
                                 [](uint64_t w) -> bool { return w != 0; });
    if (LIKELY(it != mask_.end())) {
        const auto i = static_cast<std::size_t>(std::distance(mask_.begin(), it));
        return static_cast<int64_t>(i * 64 + __builtin_ctzll(*it));
    }
    return -1;
}

int64_t OrderBook::find_prev_from(PriceTick tick) const {
    if (UNLIKELY(mask_.empty())) {
        return -1;
    }

    auto word = static_cast<std::size_t>(tick / 64);
    const auto bit_in_word = static_cast<std::size_t>(tick % 64);
    if (UNLIKELY(word >= mask_.size())) {
        word = mask_.size() - 1;
    }

    uint64_t bits = mask_[word];
    if (bit_in_word > 0) {
        const uint64_t keep_below = (1ULL << bit_in_word) - 1ULL;
        bits &= keep_below;
    } else {
        bits = 0;
    }

    if (bits != 0) {
        return static_cast<int64_t>(word * 64 + (63 - __builtin_clzll(bits)));
    }

    for (std::size_t i = word; i-- > 0;) {
        if (mask_[i]) {
            return static_cast<int64_t>(i * 64 + (63 - __builtin_clzll(mask_[i])));
        }
    }
    return -1;
}

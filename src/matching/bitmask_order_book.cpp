#include "matching/bitmask_order_book.h"

#include <algorithm>

#include "common/macros.h"

BitmaskOrderBook::BitmaskOrderBook(std::size_t max_ticks)
    : price_levels_(max_ticks, nullptr), price_level_tails_(max_ticks, nullptr),
      mask_((max_ticks + 63) / 64, 0ULL), best_first_(-1), best_last_(-1) {}

void BitmaskOrderBook::add_order_known_valid(PriceTick tick, Order *order) {
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

Order *BitmaskOrderBook::peek_order_known_valid(PriceTick tick) const {
  return price_levels_[tick];
}

Order *BitmaskOrderBook::pop_order_known_valid(PriceTick tick) {
  Order *head = price_levels_[tick];
  if (UNLIKELY(!head)) {
    return nullptr;
  }

  Order *next = head->next;
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

int64_t BitmaskOrderBook::find_first() const { return best_first_; }

int64_t BitmaskOrderBook::find_last() const { return best_last_; }

int64_t BitmaskOrderBook::find_next_from(PriceTick tick) const {
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

  const auto it =
      std::find_if(mask_.begin() + static_cast<std::ptrdiff_t>(word + 1),
                   mask_.end(), [](uint64_t w) -> bool { return w != 0; });
  if (LIKELY(it != mask_.end())) {
    const auto i = static_cast<std::size_t>(std::distance(mask_.begin(), it));
    return static_cast<int64_t>(i * 64 + __builtin_ctzll(*it));
  }
  return -1;
}

int64_t BitmaskOrderBook::find_prev_from(PriceTick tick) const {
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

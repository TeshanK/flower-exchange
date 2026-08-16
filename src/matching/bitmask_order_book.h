#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "matching/order_book.h"

// Dense tick-array strategy with a bitmask index of active price levels.
class BitmaskOrderBook final : public OrderBook {
public:
  explicit BitmaskOrderBook(std::size_t max_ticks = ORDER_BOOK_TICK_CAPACITY);

  void add_order_known_valid(PriceTick tick, Order *order) override;
  [[nodiscard]] Order *
  peek_order_known_valid(PriceTick tick) const override;
  Order *pop_order_known_valid(PriceTick tick) override;
  [[nodiscard]] int64_t find_first() const override;
  [[nodiscard]] int64_t find_last() const override;

private:
  // Finds next active tick strictly above `tick`.
  [[nodiscard]] int64_t find_next_from(PriceTick tick) const;
  // Finds previous active tick strictly below `tick`.
  [[nodiscard]] int64_t find_prev_from(PriceTick tick) const;

  std::vector<Order *> price_levels_;
  std::vector<Order *> price_level_tails_;
  std::vector<uint64_t> mask_;
  int64_t best_first_;
  int64_t best_last_;
};

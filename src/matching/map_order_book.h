#pragma once

#include <cstddef>
#include <deque>
#include <map>

#include "matching/order_book.h"

// Sparse strategy: ordered price levels containing FIFO deques.
class MapOrderBook final : public OrderBook {
public:
  explicit MapOrderBook(std::size_t max_ticks = ORDER_BOOK_TICK_CAPACITY);

  void add_order_known_valid(PriceTick tick, Order *order) override;
  [[nodiscard]] Order *
  peek_order_known_valid(PriceTick tick) const override;
  Order *pop_order_known_valid(PriceTick tick) override;
  [[nodiscard]] int64_t find_first() const override;
  [[nodiscard]] int64_t find_last() const override;

private:
  std::map<PriceTick, std::deque<Order *>> price_levels_;
};

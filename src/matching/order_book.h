#pragma once

#include <cstdint>

#include "common/types.h"

// Strategy interface used by the matching engine.
class OrderBook {
public:
  virtual ~OrderBook() = default;

  virtual void add_order_known_valid(PriceTick tick, Order *order) = 0;
  [[nodiscard]] virtual Order *
  peek_order_known_valid(PriceTick tick) const = 0;
  virtual Order *pop_order_known_valid(PriceTick tick) = 0;
  [[nodiscard]] virtual int64_t find_first() const = 0;
  [[nodiscard]] virtual int64_t find_last() const = 0;
};

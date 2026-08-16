#include "matching/map_order_book.h"

#include "common/macros.h"

MapOrderBook::MapOrderBook([[maybe_unused]] std::size_t max_ticks) {}

void MapOrderBook::add_order_known_valid(PriceTick tick, Order *order) {
  if (UNLIKELY(!order)) {
    return;
  }

  order->next = nullptr;
  price_levels_[tick].push_back(order);
}

Order *MapOrderBook::peek_order_known_valid(PriceTick tick) const {
  const auto level = price_levels_.find(tick);
  if (level == price_levels_.end() || level->second.empty()) {
    return nullptr;
  }
  return level->second.front();
}

Order *MapOrderBook::pop_order_known_valid(PriceTick tick) {
  const auto level = price_levels_.find(tick);
  if (level == price_levels_.end() || level->second.empty()) {
    return nullptr;
  }

  Order *head = level->second.front();
  level->second.pop_front();
  head->next = nullptr;
  if (level->second.empty()) {
    price_levels_.erase(level);
  }
  return head;
}

int64_t MapOrderBook::find_first() const {
  if (price_levels_.empty()) {
    return -1;
  }
  return static_cast<int64_t>(price_levels_.begin()->first);
}

int64_t MapOrderBook::find_last() const {
  if (price_levels_.empty()) {
    return -1;
  }
  return static_cast<int64_t>(price_levels_.rbegin()->first);
}

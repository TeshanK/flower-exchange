#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "common/mempool.h"
#include "common/types.h"
#include "matching/order_book.h"

class MatchingEngine {
public:
  // Binds engine to shared report pool and instrument-local books.
  MatchingEngine(
      MemPool<Order> &order_pool, MemPool<ExecutionReport> &report_pool,
      std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
          &buy_books,
      std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
          &sell_books);

  template <typename EmitReport>
  // Matches an incoming order using price-time priority and emits reports.
  void process_order(Order *incoming, EmitReport &&emit_report,
                     const char *event_timestamp = nullptr) {
    const std::size_t idx = instrument_index(incoming->instrument);
    OrderBook &opp_book =
        incoming->side == Side::BUY ? sell_books_[idx] : buy_books_[idx];
    bool matched_any = false;

    while (incoming->quantity > 0) {
      int64_t best_level = (incoming->side == Side::BUY) ? opp_book.find_first()
                                                         : opp_book.find_last();
      if (best_level < 0) {
        break;
      }

      const auto best_price = static_cast<PriceTick>(best_level);
      const bool crosses = (incoming->side == Side::BUY)
                               ? (best_price <= incoming->price)
                               : (best_price >= incoming->price);
      if (!crosses) {
        break;
      }

      Order *resting = opp_book.peek_order_known_valid(best_price);
      if (!resting) {
        continue;
      }

      const uint16_t trade_qty =
          std::min(incoming->quantity, resting->quantity);
      incoming->quantity =
          static_cast<uint16_t>(incoming->quantity - trade_qty);
      resting->quantity = static_cast<uint16_t>(resting->quantity - trade_qty);
      matched_any = true;

      const ExecStatus incoming_status =
          incoming->quantity == 0 ? ExecStatus::FILL : ExecStatus::PFILL;
      const ExecStatus resting_status =
          resting->quantity == 0 ? ExecStatus::FILL : ExecStatus::PFILL;

      const char *trade_timestamp = event_timestamp;

      emit_report(report_pool_.allocate(
          incoming->oid, incoming->coid, incoming->instrument, incoming->side,
          best_price, trade_qty, incoming_status, "", trade_timestamp));

      emit_report(report_pool_.allocate(
          resting->oid, resting->coid, resting->instrument, resting->side,
          best_price, trade_qty, resting_status, "", trade_timestamp));

      if (resting->quantity == 0) {
        Order *consumed = opp_book.pop_order_known_valid(best_price);
        (void)consumed;
        order_pool_.deallocate(resting);
      }
    }

    if (incoming->quantity > 0) {
      OrderBook &own_book =
          incoming->side == Side::BUY ? buy_books_[idx] : sell_books_[idx];
      own_book.add_order_known_valid(incoming->price, incoming);
      if (!matched_any) {
        const char *new_timestamp = event_timestamp;
        emit_report(report_pool_.allocate(incoming->oid, incoming->coid,
                                          incoming->instrument, incoming->side,
                                          incoming->price, incoming->quantity,
                                          ExecStatus::NEW, "", new_timestamp));
      }
    } else {
      order_pool_.deallocate(incoming);
    }
  }

private:
  // Converts instrument enum to book array index.
  static std::size_t instrument_index(InstrumentType instrument);

  MemPool<Order> &order_pool_;
  MemPool<ExecutionReport> &report_pool_;
  std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      &buy_books_;
  std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      &sell_books_;
};

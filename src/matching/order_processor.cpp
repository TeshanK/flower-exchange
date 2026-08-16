#include "matching/order_processor.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <thread>

#include "common/macros.h"
#include "common/spsc_utils.h"
#include "common/validator.h"

namespace {

std::size_t copy_text(char *destination, std::size_t destination_size,
                      const char *source) {
  if (UNLIKELY(!destination || destination_size == 0)) {
    return 0;
  }
  destination[0] = '\0';
  if (UNLIKELY(!source)) {
    return 0;
  }

  const std::size_t max_copy = destination_size - 1;
  std::size_t copy_length = 0;
  while (copy_length < max_copy && source[copy_length] != '\0') {
    destination[copy_length] = source[copy_length];
    ++copy_length;
  }
  destination[copy_length] = '\0';
  return copy_length;
}

void format_order_id(char *order_id, std::size_t order_id_size, uint64_t id) {
  if (UNLIKELY(!order_id || order_id_size < 5)) {
    return;
  }
  order_id[0] = 'o';
  order_id[1] = 'r';
  order_id[2] = 'd';
  auto result =
      std::to_chars(order_id + 3, order_id + order_id_size - 1, id);
  if (UNLIKELY(result.ec == std::errc())) {
    *result.ptr = '\0';
    return;
  }
  order_id[0] = '\0';
}

void fill_outbound_fields(OutboundReportMsg &out,
                          const ExecutionReport *report,
                          const char *instrument_text, int side) {
  out.oid_len =
      static_cast<uint8_t>(copy_text(out.oid, sizeof(out.oid), report->oid));
  out.coid_len =
      static_cast<uint8_t>(copy_text(out.coid, sizeof(out.coid), report->coid));
  out.instrument_len = static_cast<uint8_t>(copy_text(
      out.instrument, sizeof(out.instrument), instrument_text));
  out.side = side;
  out.exec_status_len = static_cast<uint8_t>(
      copy_text(out.exec_status, sizeof(out.exec_status),
                exec_status_to_string(report->status)));
  out.quantity = report->quantity;
  out.reason_len = static_cast<uint8_t>(
      copy_text(out.reason, sizeof(out.reason), report->reason));
  out.timestamp_len = static_cast<uint8_t>(
      copy_text(out.timestamp, sizeof(out.timestamp), report->timestamp));
}

} // namespace

OrderProcessor::OrderProcessor()
    : order_pool_(RuntimeConfig::kOrderPoolInitialCapacity),
      report_pool_(RuntimeConfig::kReportPoolInitialCapacity),
      buy_books_{BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                 BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity)},
      sell_books_{BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                  BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                  BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                  BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity),
                  BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity)} {
  timestamp_cache_.start();
  rebuild_matcher();
}

OrderProcessor::~OrderProcessor() {
  matcher_.reset();
  timestamp_cache_.stop();
}

void OrderProcessor::reset() {
  next_oid_ = 1;
  std::fill(buy_books_.begin(), buy_books_.end(),
            BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity));
  std::fill(sell_books_.begin(), sell_books_.end(),
            BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity));
  rebuild_matcher();
}

void OrderProcessor::rebuild_matcher() {
  matcher_ = std::make_unique<MatchingEngine>(order_pool_, report_pool_,
                                              buy_books_, sell_books_);
}

void OrderProcessor::consume(InboundOrderQueue &inbound_queue,
                             OutboundReportQueue &outbound_queue,
                             PipelineState &state) {
  const auto matcher_start = std::chrono::steady_clock::now();
  uint64_t consumed = 0;

  bool got_end_of_stream = false;
  int pop_spin = 0;
  while (!got_end_of_stream || !state.producer_done.load()) {
    InboundOrderMsg message{};
    if (!inbound_queue.pop(message)) {
      if (pop_spin++ < 1000) {
        __builtin_ia32_pause();
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        pop_spin = 0;
      }
      continue;
    }
    pop_spin = 0;

    if (message.end_of_stream) {
      got_end_of_stream = true;
      continue;
    }
    ++consumed;

    std::array<char, 20> event_timestamp{};
    timestamp_cache_.snapshot(event_timestamp.data(), event_timestamp.size());

    std::array<char, 32> order_id{};
    format_order_id(order_id.data(), order_id.size(), next_oid_++);

    InstrumentType instrument = InstrumentType::COUNT;
    parse_instrument(message.instrument, instrument);
    const auto validation = validate_order(instrument, message.side,
                                           message.price, message.quantity);

    if (UNLIKELY(!validation.first)) {
      ExecutionReport *reject = report_pool_.allocate(
          order_id.data(), message.coid, InstrumentType::COUNT,
          (message.side == static_cast<int>(Side::SELL) ? Side::SELL
                                                        : Side::BUY),
          0,
          static_cast<uint16_t>(message.quantity > 0 ? message.quantity : 0),
          ExecStatus::REJECTED, validation.second, event_timestamp.data());

      OutboundReportMsg output{};
      fill_outbound_fields(output, reject, message.instrument, message.side);
      output.price_text_len = static_cast<uint8_t>(format_price_to_buffer(
          message.price, output.price_text, sizeof(output.price_text)));
      output.seq = message.seq;

      spin_until_success(
          [&]() -> bool { return outbound_queue.push(output); }, 100);
      report_pool_.deallocate(reject);
      continue;
    }

    const PriceTick price_ticks = double_to_ticks(message.price);
    Order *order = order_pool_.allocate(
        order_id.data(), message.coid, instrument,
        message.side == static_cast<int>(Side::BUY) ? Side::BUY : Side::SELL,
        price_ticks, static_cast<uint16_t>(message.quantity));

    matcher_->process_order(
        order,
        [&](ExecutionReport *report) -> void {
          OutboundReportMsg output{};
          fill_outbound_fields(output, report,
                               instrument_to_string(report->instrument),
                               static_cast<int>(report->side));
          output.price_text_len = static_cast<uint8_t>(format_ticks_to_buffer(
              report->price, output.price_text, sizeof(output.price_text)));
          output.seq = message.seq;

          spin_until_success(
              [&]() -> bool { return outbound_queue.push(output); }, 100);
          report_pool_.deallocate(report);
        },
        event_timestamp.data());
  }

  const auto matcher_end = std::chrono::steady_clock::now();
  state.matcher_ns.store(static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(matcher_end -
                                                           matcher_start)
          .count()));
  state.consumed_orders.store(consumed);
  state.matcher_done.store(true);
}

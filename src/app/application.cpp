#include "app/application.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <print>

#include "common/macros.h"
#include "common/spsc_utils.h"
#include "common/thread_utils.h"
#include "common/validator.h"
#include "io/csv_reader.h"
#include "io/csv_report_writer.h"
#include "app/utils.h"

Application::Application()
    : initialized_(false), next_oid_(1),
      order_pool_(RuntimeConfig::kOrderPoolInitialCapacity),
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
                  BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity)} {}

Application::~Application() { shutdown(); }

void Application::init() {
  if (initialized_) {
    return;
  }

  timestamp_cache_.start();
  rebuild_matcher();
  initialized_ = true;
}

void Application::rebuild_matcher() {
  matcher_ = std::make_unique<MatchingEngine>(order_pool_, report_pool_,
                                              buy_books_, sell_books_);
}

void Application::shutdown() {
  if (!initialized_) {
    return;
  }
  matcher_.reset();
  timestamp_cache_.stop();
  initialized_ = false;
}

void Application::io_producer(const std::string &input_path,
                              std::atomic<bool> &producer_done,
                              std::atomic<uint64_t> &produced_orders,
                              std::atomic<uint64_t> &producer_ns) {
  const auto producer_start = std::chrono::steady_clock::now();
  uint64_t produced = 0;

  std::ifstream file(input_path);
  // Tune input buffer to 256 KB for faster bulk reads
  std::array<char, 262144> file_buffer{};
  file.rdbuf()->pubsetbuf(file_buffer.data(),
                          static_cast<std::streamsize>(file_buffer.size()));

  if (!file.is_open()) {
    std::println(std::cerr, "Unable to open input file: {}", input_path);
    InboundOrderMsg eos{};
    eos.end_of_stream = true;
    spin_until_success([&]() -> bool { return inbound_queue_.push(eos); }, 100);
    producer_done.store(true);
    return;
  }

  bool first_row = true;
  uint64_t seq = 1;
  for (const CSVRow &row : CSVRange(file)) {
    if (first_row) {
      first_row = false;
      continue;
    }
    if (row.size() < 5) {
      continue;
    }

    InboundOrderMsg msg{};
    msg.end_of_stream = false;
    copy_text(msg.coid, sizeof(msg.coid), row[0]);
    copy_text(msg.instrument, sizeof(msg.instrument), row[1]);
    msg.side = string_view_to_int(row[2]);
    msg.quantity = string_view_to_int(row[3]);
    msg.price = string_view_to_double(row[4]);
    msg.seq = seq++;
    ++produced;

    spin_until_success([&]() -> bool { return inbound_queue_.push(msg); }, 100);
  }

  InboundOrderMsg eos{};
  eos.end_of_stream = true;
  spin_until_success([&]() -> bool { return inbound_queue_.push(eos); }, 100);

  const auto producer_end = std::chrono::steady_clock::now();
  producer_ns.store(static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(producer_end -
                                                           producer_start)
          .count()));
  produced_orders.store(produced);
  producer_done.store(true);
}

void Application::matching_consumer(std::atomic<bool> &producer_done,
                                    std::atomic<bool> &matcher_done,
                                    std::atomic<uint64_t> &consumed_orders,
                                    std::atomic<uint64_t> &matcher_ns) {
  const auto matcher_start = std::chrono::steady_clock::now();
  uint64_t consumed = 0;

  bool got_eos = false;
  int pop_spin = 0;
  while (!got_eos || !producer_done.load()) {
    InboundOrderMsg msg{};
    if (!inbound_queue_.pop(msg)) {
      if (pop_spin++ < 1000) {
        __builtin_ia32_pause();
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        pop_spin = 0;
      }
      continue;
    }
    pop_spin = 0;

    if (msg.end_of_stream) {
      got_eos = true;
      continue;
    }
    ++consumed;

    std::array<char, 20> event_timestamp{};
    timestamp_cache_.snapshot(event_timestamp.data(), event_timestamp.size());

    std::array<char, 32> oid{};
    format_order_id(oid.data(), oid.size(), next_oid_++);

    InstrumentType instrument = InstrumentType::COUNT;
    parse_instrument(msg.instrument, instrument);
    auto validation =
        validate_order(instrument, msg.side, msg.price, msg.quantity);

    if (UNLIKELY(!validation.first)) {
      ExecutionReport *reject = report_pool_.allocate(
          oid.data(), msg.coid, InstrumentType::COUNT,
          (msg.side == static_cast<int>(Side::SELL) ? Side::SELL : Side::BUY),
          0, static_cast<uint16_t>(msg.quantity > 0 ? msg.quantity : 0),
          ExecStatus::REJECTED, validation.second, event_timestamp.data());

      OutboundReportMsg out{};
      fill_outbound_common_fields(out, reject, msg.instrument, msg.side);
      out.price_text_len = static_cast<uint8_t>(format_price_to_buffer(
          msg.price, out.price_text, sizeof(out.price_text)));
      out.seq = msg.seq;

      spin_until_success([&]() -> bool { return outbound_queue_.push(out); },
                         100);
      report_pool_.deallocate(reject);
      continue;
    }

    const PriceTick price_ticks = double_to_ticks(msg.price);

    Order *order = order_pool_.allocate(
        oid.data(), msg.coid, instrument,
        msg.side == static_cast<int>(Side::BUY) ? Side::BUY : Side::SELL,
        price_ticks, static_cast<uint16_t>(msg.quantity));

    matcher_->process_order(
        order,
        [&](ExecutionReport *report) -> void {
          OutboundReportMsg out{};
          fill_outbound_common_fields(out, report,
                                      instrument_to_string(report->instrument),
                                      static_cast<int>(report->side));
          out.price_text_len = static_cast<uint8_t>(format_ticks_to_buffer(
              report->price, out.price_text, sizeof(out.price_text)));
          out.seq = msg.seq;

          spin_until_success(
              [&]() -> bool { return outbound_queue_.push(out); }, 100);
          report_pool_.deallocate(report);
        },
        event_timestamp.data());
  }

  const auto matcher_end = std::chrono::steady_clock::now();
  matcher_ns.store(static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(matcher_end -
                                                           matcher_start)
          .count()));
  consumed_orders.store(consumed);
  matcher_done.store(true);
}

void Application::process_file(const std::string &input_path) {
  const auto total_start = std::chrono::steady_clock::now();
  next_oid_ = 1;

  while (!inbound_queue_.empty()) {
    InboundOrderMsg tmp{};
    inbound_queue_.pop(tmp);
  }
  while (!outbound_queue_.empty()) {
    OutboundReportMsg tmp{};
    outbound_queue_.pop(tmp);
  }

  std::fill(buy_books_.begin(), buy_books_.end(),
            BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity));
  std::fill(sell_books_.begin(), sell_books_.end(),
            BitmaskOrderBook(RuntimeConfig::kOrderBookTickCapacity));
  rebuild_matcher();

  CsvReportWriter report_writer;
  if (!report_writer.prepare(input_path)) {
    return;
  }
  const std::string output_path = report_writer.output_path();

  std::atomic<bool> producer_done{false};
  std::atomic<bool> matcher_done{false};
  std::atomic<uint64_t> produced_orders{0};
  std::atomic<uint64_t> consumed_orders{0};
  std::atomic<uint64_t> producer_ns{0};
  std::atomic<uint64_t> matcher_ns{0};

  auto producer =
      createAndStartThread(0, "io-producer", &Application::io_producer, this,
                           input_path, std::ref(producer_done),
                           std::ref(produced_orders), std::ref(producer_ns));

  auto matcher =
      createAndStartThread(1, "matcher", &Application::matching_consumer, this,
                           std::ref(producer_done), std::ref(matcher_done),
                           std::ref(consumed_orders), std::ref(matcher_ns));

  const uint64_t writer_ns = report_writer.drain(outbound_queue_, matcher_done);

  if (producer->joinable()) {
    producer->join();
    producer_done.store(true);
  }

  if (matcher->joinable()) {
    matcher->join();
    matcher_done.store(true);
  }

  const auto write_end = std::chrono::steady_clock::now();
  const double total_sec =
      std::chrono::duration_cast<std::chrono::duration<double>>(write_end -
                                                                total_start)
          .count();
  const double producer_sec =
      static_cast<double>(producer_ns.load()) / 1'000'000'000.0;
  const double matcher_sec =
      static_cast<double>(matcher_ns.load()) / 1'000'000'000.0;
  const double write_sec = static_cast<double>(writer_ns) / 1'000'000'000.0;
  const uint64_t orders = produced_orders.load();
  const uint64_t matched = consumed_orders.load();
  const double orders_per_sec =
      (total_sec > 0.0) ? (static_cast<double>(orders) / total_sec) : 0.0;

  std::println("Wrote reports to {}", output_path);
  std::println("PERF orders={} consumed={} total_sec={} orders_per_sec={}",
               orders, matched, total_sec, orders_per_sec);
  std::println(" producer_sec={} matcher_sec={} write_sec={}", producer_sec,
               matcher_sec, write_sec);
} // namespace app

void Application::run() {
  init();

  std::string user_input;
  while (true) {
    std::getline(std::cin, user_input);
    if (user_input == "QUIT") {
      break;
    }

    std::stringstream ss(user_input);
    std::string cmd;
    std::string path;
    ss >> cmd >> path;

    if (cmd == "PROCESS" && !path.empty()) {
      process_file(path);
    }
  }
}

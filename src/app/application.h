#pragma once

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include "app/pipeline_types.h"
#include "common/mempool.h"
#include "common/timestamp_cache.h"
#include "common/types.h"
#include "matching/bitmask_order_book.h"
#include "matching/matching_engine.h"

class Application {
public:
  // Constructs pools/books and default runtime state.
  Application();
  // Ensures clean shutdown.
  ~Application();

  // Runs interactive command loop (PROCESS/QUIT).
  void run();

private:
  // One-time component initialization.
  void init();
  // (Re)creates matcher with current pools/books.
  void rebuild_matcher();
  // Component teardown.
  void shutdown();

  // Processes one input CSV into one output report file.
  void process_file(const std::string &input_path);
  // Producer thread body: parse CSV and enqueue inbound messages.
  void io_producer(const std::string &input_path,
                   std::atomic<bool> &producer_done,
                   std::atomic<uint64_t> &produced_orders,
                   std::atomic<uint64_t> &producer_ns);
  // Consumer thread body: validate/match and enqueue report messages.
  void matching_consumer(std::atomic<bool> &producer_done,
                         std::atomic<bool> &matcher_done,
                         std::atomic<uint64_t> &consumed_orders,
                         std::atomic<uint64_t> &matcher_ns);

  bool initialized_;
  uint64_t next_oid_;

  MemPool<Order> order_pool_;
  MemPool<ExecutionReport> report_pool_;

  std::array<BitmaskOrderBook,
             static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books_;
  std::array<BitmaskOrderBook,
             static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books_;
  std::unique_ptr<MatchingEngine> matcher_;
  TimestampCache timestamp_cache_;

  InboundOrderQueue inbound_queue_;
  OutboundReportQueue outbound_queue_;
};

#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "app/pipeline_types.h"
#include "common/mempool.h"
#include "common/timestamp_cache.h"
#include "common/types.h"
#include "matching/bitmask_order_book.h"
#include "matching/matching_engine.h"

class OrderProcessor final {
public:
  OrderProcessor();
  ~OrderProcessor();

  OrderProcessor(const OrderProcessor &) = delete;
  OrderProcessor &operator=(const OrderProcessor &) = delete;

  // Clears all per-file order state and resets exchange order IDs.
  void reset();

  // Validates and matches inbound orders until the producer reaches EOS.
  void consume(InboundOrderQueue &inbound_queue,
               OutboundReportQueue &outbound_queue, PipelineState &state);

private:
  void rebuild_matcher();

  uint64_t next_oid_{1};
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
};

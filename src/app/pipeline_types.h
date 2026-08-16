#pragma once

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <cstdint>

#include "common/runtime_config.h"

// Message payload produced by the CSV reader thread and consumed by the
// matching thread.
struct InboundOrderMsg {
  bool end_of_stream;
  char coid[64];
  char instrument[32];
  int side;
  int quantity;
  double price;
  uint64_t seq;
};

// Message payload produced by the matching thread and consumed by the writer.
// String lengths are precomputed to avoid repeated strlen in the hot path.
struct OutboundReportMsg {
  char oid[32];
  uint8_t oid_len;
  char coid[64];
  uint8_t coid_len;
  char instrument[32];
  uint8_t instrument_len;
  int side;
  char exec_status[16];
  uint8_t exec_status_len;
  int quantity;
  char price_text[24];
  uint8_t price_text_len;
  char reason[64];
  uint8_t reason_len;
  char timestamp[20];
  uint8_t timestamp_len;
  uint64_t seq;
};

using InboundOrderQueue = boost::lockfree::spsc_queue<
    InboundOrderMsg,
    boost::lockfree::capacity<RuntimeConfig::kInboundQueueCapacity>>;

using OutboundReportQueue = boost::lockfree::spsc_queue<
    OutboundReportMsg,
    boost::lockfree::capacity<RuntimeConfig::kOutboundQueueCapacity>>;

struct PipelineState final {
  std::atomic<bool> producer_done{false};
  std::atomic<bool> matcher_done{false};
  std::atomic<uint64_t> produced_orders{0};
  std::atomic<uint64_t> consumed_orders{0};
  std::atomic<uint64_t> producer_ns{0};
  std::atomic<uint64_t> matcher_ns{0};
};

#pragma once

#include "common/mempool.h"
#include "common/types.h"
#include "matching/matching_engine.h"
#include "matching/order_book.h"

#include <array>
#include <atomic>
#include <memory>
#include <string>

#include <boost/lockfree/spsc_queue.hpp>

// Message payload produced by CSV reader thread and consumed by matcher thread.
struct InboundOrderMsg {
    bool end_of_stream;
    char coid[64];
    char instrument[32];
    int side;
    int quantity;
    double price;
    uint64_t seq;
};

// Message payload produced by matcher thread and consumed by writer loop.
// String lengths are precomputed to avoid repeated strlen in hot output path.
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
    void process_file(const std::string& input_path);
    // Builds output path from input filename stem.
    static std::string derive_output_path(const std::string& input_path);

    // Producer thread body: parse CSV and enqueue inbound messages.
    void io_producer(const std::string& input_path,
                     std::atomic<bool>& producer_done,
                     std::atomic<uint64_t>& produced_orders,
                     std::atomic<uint64_t>& producer_ns);
    // Consumer thread body: validate/match and enqueue report messages.
    void matching_consumer(std::atomic<bool>& producer_done,
                           std::atomic<bool>& matcher_done,
                           std::atomic<uint64_t>& consumed_orders,
                           std::atomic<uint64_t>& matcher_ns);

    bool initialized_;
    uint64_t next_oid_;

    MemPool<Order> order_pool_;
    MemPool<ExecutionReport> report_pool_;

    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books_;
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books_;
    std::unique_ptr<MatchingEngine> matcher_;

    boost::lockfree::spsc_queue<InboundOrderMsg, boost::lockfree::capacity<1 << 16>> inbound_queue_;
    boost::lockfree::spsc_queue<OutboundReportMsg, boost::lockfree::capacity<1 << 17>> outbound_queue_;
};

#include "matching/matching_engine.h"

MatchingEngine::MatchingEngine(
        MemPool<Order>& order_pool,
    MemPool<ExecutionReport>& report_pool,
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>& buy_books,
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>& sell_books)
        : order_pool_(order_pool),
            report_pool_(report_pool),
            buy_books_(buy_books),
            sell_books_(sell_books) {}

std::size_t MatchingEngine::instrument_index(InstrumentType instrument) {
    return static_cast<std::size_t>(instrument);
}

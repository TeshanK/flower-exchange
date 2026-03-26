#include "common/mempool.h"
#include "matching/matching_engine.h"

#include <gtest/gtest.h>

#include <vector>

TEST(MatchingEngineTest, FullFill) {
    MemPool<Order> order_pool(32);
    MemPool<ExecutionReport> rep_pool(32);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 10, 100);
    Order* buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::BUY, 10, 100);

    int count = 0;
    engine.process_order(sell, [&](ExecutionReport*) -> void { ++count; });
    engine.process_order(buy, [&](ExecutionReport*) -> void { ++count; });

    EXPECT_EQ(count, 3);
}

TEST(MatchingEngineTest, PartialFillDoesNotEmitNew) {
    MemPool<Order> order_pool(32);
    MemPool<ExecutionReport> rep_pool(32);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* resting = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 45, 100);
    Order* incoming = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::BUY, 45, 200);

    std::vector<ExecStatus> statuses;
    engine.process_order(resting, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });
    engine.process_order(incoming, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });

    ASSERT_EQ(statuses.size(), 3u);
    EXPECT_EQ(statuses[0], ExecStatus::NEW);
    EXPECT_EQ(statuses[1], ExecStatus::PFILL);
    EXPECT_EQ(statuses[2], ExecStatus::FILL);
}

TEST(MatchingEngineTest, MultiLevelSweep) {
    MemPool<Order> order_pool(64);
    MemPool<ExecutionReport> rep_pool(64);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* bid1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::BUY, 55, 100);
    Order* bid2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::BUY, 65, 100);
    Order* sell = order_pool.allocate("ord3", "c3", InstrumentType::ROSE, Side::SELL, 1, 300);

    int reports = 0;
    engine.process_order(bid1, [&](ExecutionReport*) -> void { ++reports; });
    engine.process_order(bid2, [&](ExecutionReport*) -> void { ++reports; });
    engine.process_order(sell, [&](ExecutionReport*) -> void { ++reports; });

    EXPECT_EQ(reports, 6);
}

TEST(MatchingEngineTest, NoCrossEmitsOnlyNew) {
    MemPool<Order> order_pool(32);
    MemPool<ExecutionReport> rep_pool(32);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);
    Order* sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 55, 100);
    Order* buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::BUY, 45, 100);

    std::vector<ExecStatus> statuses;
    engine.process_order(sell, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });
    engine.process_order(buy, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });

    ASSERT_EQ(statuses.size(), 2u);
    EXPECT_EQ(statuses[0], ExecStatus::NEW);
    EXPECT_EQ(statuses[1], ExecStatus::NEW);
}

TEST(MatchingEngineTest, CrossInstrumentIsolation) {
    MemPool<Order> order_pool(32);
    MemPool<ExecutionReport> rep_pool(32);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* rose_sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 50, 100);
    Order* tulip_buy = order_pool.allocate("ord2", "c2", InstrumentType::TULIP, Side::BUY, 100, 100);

    int reports = 0;
    engine.process_order(rose_sell, [&](ExecutionReport*) -> void { ++reports; });
    engine.process_order(tulip_buy, [&](ExecutionReport*) -> void { ++reports; });

    EXPECT_EQ(reports, 2);
}

TEST(MatchingEngineTest, EqualPriceCrossesAndDoesNotRest) {
    MemPool<Order> order_pool(32);
    MemPool<ExecutionReport> rep_pool(32);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);
    Order* sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 50, 100);
    Order* buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::BUY, 50, 100);

    std::vector<ExecStatus> statuses;
    engine.process_order(sell, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });
    engine.process_order(buy, [&](ExecutionReport* rep) -> void { statuses.push_back(rep->status); });

    ASSERT_EQ(statuses.size(), 3u);
    EXPECT_EQ(statuses[0], ExecStatus::NEW);
    EXPECT_EQ(statuses[1], ExecStatus::FILL);
    EXPECT_EQ(statuses[2], ExecStatus::FILL);
}

TEST(MatchingEngineTest, SamePriceLevelFifoOrderIsStable) {
    MemPool<Order> order_pool(128);
    MemPool<ExecutionReport> rep_pool(128);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* s1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 50, 10);
    Order* s2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::SELL, 50, 10);
    Order* s3 = order_pool.allocate("ord3", "c3", InstrumentType::ROSE, Side::SELL, 50, 10);
    Order* buy = order_pool.allocate("ord4", "cb", InstrumentType::ROSE, Side::BUY, 50, 30);

    std::vector<std::string> passive_fill_ids;
    engine.process_order(s1, [&](ExecutionReport*) -> void {});
    engine.process_order(s2, [&](ExecutionReport*) -> void {});
    engine.process_order(s3, [&](ExecutionReport*) -> void {});

    engine.process_order(buy, [&](ExecutionReport* rep) -> void {
        if (rep->status == ExecStatus::FILL && rep->side == Side::SELL) {
            passive_fill_ids.emplace_back(rep->oid);
        }
    });

    ASSERT_EQ(passive_fill_ids.size(), 3u);
    EXPECT_EQ(passive_fill_ids[0], "ord1");
    EXPECT_EQ(passive_fill_ids[1], "ord2");
    EXPECT_EQ(passive_fill_ids[2], "ord3");
}

TEST(MatchingEngineTest, PartiallyFilledRestingOrderKeepsTimePriority) {
    MemPool<Order> order_pool(128);
    MemPool<ExecutionReport> rep_pool(128);
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> buy_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};
    std::array<OrderBook, static_cast<std::size_t>(InstrumentType::COUNT)> sell_books{
        OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024), OrderBook(1024)};

    MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

    Order* s1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE, Side::SELL, 45, 200);
    Order* s2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE, Side::SELL, 45, 200);
    Order* b1 = order_pool.allocate("ord3", "cb1", InstrumentType::ROSE, Side::BUY, 50, 100);
    Order* b2 = order_pool.allocate("ord4", "cb2", InstrumentType::ROSE, Side::BUY, 55, 100);

    std::vector<std::string> passive_fill_ids;
    engine.process_order(s1, [&](ExecutionReport*) -> void {});
    engine.process_order(s2, [&](ExecutionReport*) -> void {});

    engine.process_order(b1, [&](ExecutionReport* rep) -> void {
        if ((rep->status == ExecStatus::PFILL || rep->status == ExecStatus::FILL) &&
            rep->side == Side::SELL) {
            passive_fill_ids.emplace_back(rep->oid);
        }
    });

    engine.process_order(b2, [&](ExecutionReport* rep) -> void {
        if ((rep->status == ExecStatus::PFILL || rep->status == ExecStatus::FILL) &&
            rep->side == Side::SELL) {
            passive_fill_ids.emplace_back(rep->oid);
        }
    });

    ASSERT_EQ(passive_fill_ids.size(), 2u);
    EXPECT_EQ(passive_fill_ids[0], "ord1");
    EXPECT_EQ(passive_fill_ids[1], "ord1");
}

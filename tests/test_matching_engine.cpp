#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "common/mempool.h"
#include "matching/bitmask_order_book.h"
#include "matching/map_order_book.h"
#include "matching/matching_engine.h"

namespace {

struct ReportSnapshot {
  std::string oid;
  std::string coid;
  InstrumentType instrument;
  Side side;
  PriceTick price;
  uint16_t quantity;
  ExecStatus status;
  std::string reason;
  std::string timestamp;

  bool operator==(const ReportSnapshot &) const = default;
};

template <typename BookStrategy>
std::vector<ReportSnapshot> run_matching_scenario() {
  MemPool<Order> order_pool(128);
  MemPool<ExecutionReport> report_pool(128);
  std::array<BookStrategy, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BookStrategy(1024), BookStrategy(1024), BookStrategy(1024),
                BookStrategy(1024), BookStrategy(1024)};
  std::array<BookStrategy, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BookStrategy(1024), BookStrategy(1024), BookStrategy(1024),
                 BookStrategy(1024), BookStrategy(1024)};
  MatchingEngine engine(order_pool, report_pool, buy_books, sell_books);
  std::vector<ReportSnapshot> output;

  const auto submit = [&](const char *oid, const char *coid,
                          InstrumentType instrument, Side side,
                          PriceTick price, uint16_t quantity) {
    Order *order =
        order_pool.allocate(oid, coid, instrument, side, price, quantity);
    engine.process_order(
        order,
        [&](ExecutionReport *report) {
          output.push_back({report->oid, report->coid, report->instrument,
                            report->side, report->price, report->quantity,
                            report->status, report->reason, report->timestamp});
          report_pool.deallocate(report);
        },
        "20260816-120000.000");
  };

  // Same-price FIFO, multi-level sweeps, partial fills, both sides, and
  // instrument isolation are all exercised in one deterministic stream.
  submit("ord1", "c1", InstrumentType::ROSE, Side::SELL, 50, 100);
  submit("ord2", "c2", InstrumentType::ROSE, Side::SELL, 50, 150);
  submit("ord3", "c3", InstrumentType::ROSE, Side::SELL, 55, 100);
  submit("ord4", "c4", InstrumentType::ROSE, Side::BUY, 50, 120);
  submit("ord5", "c5", InstrumentType::ROSE, Side::BUY, 60, 200);
  submit("ord6", "c6", InstrumentType::TULIP, Side::BUY, 70, 100);
  submit("ord7", "c7", InstrumentType::TULIP, Side::BUY, 75, 50);
  submit("ord8", "c8", InstrumentType::TULIP, Side::SELL, 60, 120);
  submit("ord9", "c9", InstrumentType::ROSE, Side::BUY, 45, 50);
  submit("ord10", "c10", InstrumentType::ROSE, Side::SELL, 40, 70);
  submit("ord11", "c11", InstrumentType::ROSE, Side::BUY, 100, 200);

  return output;
}

} // namespace

TEST(MatchingEngineTest, FullFill) {
  MemPool<Order> order_pool(32);
  MemPool<ExecutionReport> rep_pool(32);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                    Side::SELL, 10, 100);
  Order *buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                   Side::BUY, 10, 100);

  int count = 0;
  engine.process_order(sell, [&](ExecutionReport *) -> void { ++count; });
  engine.process_order(buy, [&](ExecutionReport *) -> void { ++count; });

  EXPECT_EQ(count, 3);
}

TEST(MatchingEngineTest, PartialFillDoesNotEmitNew) {
  MemPool<Order> order_pool(32);
  MemPool<ExecutionReport> rep_pool(32);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *resting = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                       Side::SELL, 45, 100);
  Order *incoming = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                        Side::BUY, 45, 200);

  std::vector<ExecStatus> statuses;
  engine.process_order(resting, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });
  engine.process_order(incoming, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });

  ASSERT_EQ(statuses.size(), 3u);
  EXPECT_EQ(statuses[0], ExecStatus::NEW);
  EXPECT_EQ(statuses[1], ExecStatus::PFILL);
  EXPECT_EQ(statuses[2], ExecStatus::FILL);
}

TEST(MatchingEngineTest, MultiLevelSweep) {
  MemPool<Order> order_pool(64);
  MemPool<ExecutionReport> rep_pool(64);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *bid1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                    Side::BUY, 55, 100);
  Order *bid2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                    Side::BUY, 65, 100);
  Order *sell = order_pool.allocate("ord3", "c3", InstrumentType::ROSE,
                                    Side::SELL, 1, 300);

  int reports = 0;
  engine.process_order(bid1, [&](ExecutionReport *) -> void { ++reports; });
  engine.process_order(bid2, [&](ExecutionReport *) -> void { ++reports; });
  engine.process_order(sell, [&](ExecutionReport *) -> void { ++reports; });

  EXPECT_EQ(reports, 6);
}

TEST(MatchingEngineTest, NoCrossEmitsOnlyNew) {
  MemPool<Order> order_pool(32);
  MemPool<ExecutionReport> rep_pool(32);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);
  Order *sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                    Side::SELL, 55, 100);
  Order *buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                   Side::BUY, 45, 100);

  std::vector<ExecStatus> statuses;
  engine.process_order(sell, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });
  engine.process_order(buy, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });

  ASSERT_EQ(statuses.size(), 2u);
  EXPECT_EQ(statuses[0], ExecStatus::NEW);
  EXPECT_EQ(statuses[1], ExecStatus::NEW);
}

TEST(MatchingEngineTest, CrossInstrumentIsolation) {
  MemPool<Order> order_pool(32);
  MemPool<ExecutionReport> rep_pool(32);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *rose_sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                         Side::SELL, 50, 100);
  Order *tulip_buy = order_pool.allocate("ord2", "c2", InstrumentType::TULIP,
                                         Side::BUY, 100, 100);

  int reports = 0;
  engine.process_order(rose_sell,
                       [&](ExecutionReport *) -> void { ++reports; });
  engine.process_order(tulip_buy,
                       [&](ExecutionReport *) -> void { ++reports; });

  EXPECT_EQ(reports, 2);
}

TEST(MatchingEngineTest, EqualPriceCrossesAndDoesNotRest) {
  MemPool<Order> order_pool(32);
  MemPool<ExecutionReport> rep_pool(32);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);
  Order *sell = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                    Side::SELL, 50, 100);
  Order *buy = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                   Side::BUY, 50, 100);

  std::vector<ExecStatus> statuses;
  engine.process_order(sell, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });
  engine.process_order(buy, [&](ExecutionReport *rep) -> void {
    statuses.push_back(rep->status);
  });

  ASSERT_EQ(statuses.size(), 3u);
  EXPECT_EQ(statuses[0], ExecStatus::NEW);
  EXPECT_EQ(statuses[1], ExecStatus::FILL);
  EXPECT_EQ(statuses[2], ExecStatus::FILL);
}

TEST(MatchingEngineTest, SamePriceLevelFifoOrderIsStable) {
  MemPool<Order> order_pool(128);
  MemPool<ExecutionReport> rep_pool(128);
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *s1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                  Side::SELL, 50, 10);
  Order *s2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                  Side::SELL, 50, 10);
  Order *s3 = order_pool.allocate("ord3", "c3", InstrumentType::ROSE,
                                  Side::SELL, 50, 10);
  Order *buy = order_pool.allocate("ord4", "cb", InstrumentType::ROSE,
                                   Side::BUY, 50, 30);

  std::vector<std::string> passive_fill_ids;
  engine.process_order(s1, [&](ExecutionReport *) -> void {});
  engine.process_order(s2, [&](ExecutionReport *) -> void {});
  engine.process_order(s3, [&](ExecutionReport *) -> void {});

  engine.process_order(buy, [&](ExecutionReport *rep) -> void {
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
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      buy_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                BitmaskOrderBook(1024), BitmaskOrderBook(1024)};
  std::array<BitmaskOrderBook, static_cast<std::size_t>(InstrumentType::COUNT)>
      sell_books{BitmaskOrderBook(1024), BitmaskOrderBook(1024), BitmaskOrderBook(1024),
                 BitmaskOrderBook(1024), BitmaskOrderBook(1024)};

  MatchingEngine engine(order_pool, rep_pool, buy_books, sell_books);

  Order *s1 = order_pool.allocate("ord1", "c1", InstrumentType::ROSE,
                                  Side::SELL, 45, 200);
  Order *s2 = order_pool.allocate("ord2", "c2", InstrumentType::ROSE,
                                  Side::SELL, 45, 200);
  Order *b1 = order_pool.allocate("ord3", "cb1", InstrumentType::ROSE,
                                  Side::BUY, 50, 100);
  Order *b2 = order_pool.allocate("ord4", "cb2", InstrumentType::ROSE,
                                  Side::BUY, 55, 100);

  std::vector<std::string> passive_fill_ids;
  engine.process_order(s1, [&](ExecutionReport *) -> void {});
  engine.process_order(s2, [&](ExecutionReport *) -> void {});

  engine.process_order(b1, [&](ExecutionReport *rep) -> void {
    if ((rep->status == ExecStatus::PFILL || rep->status == ExecStatus::FILL) &&
        rep->side == Side::SELL) {
      passive_fill_ids.emplace_back(rep->oid);
    }
  });

  engine.process_order(b2, [&](ExecutionReport *rep) -> void {
    if ((rep->status == ExecStatus::PFILL || rep->status == ExecStatus::FILL) &&
        rep->side == Side::SELL) {
      passive_fill_ids.emplace_back(rep->oid);
    }
  });

  ASSERT_EQ(passive_fill_ids.size(), 2u);
  EXPECT_EQ(passive_fill_ids[0], "ord1");
  EXPECT_EQ(passive_fill_ids[1], "ord1");
}

TEST(MatchingEngineStrategyTest, MapAndBitmaskProduceIdenticalReports) {
  const auto bitmask_output = run_matching_scenario<BitmaskOrderBook>();
  const auto map_output = run_matching_scenario<MapOrderBook>();

  ASSERT_FALSE(bitmask_output.empty());
  EXPECT_EQ(map_output, bitmask_output);
}

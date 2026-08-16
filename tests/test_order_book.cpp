#include <gtest/gtest.h>

#include <type_traits>

#include "matching/bitmask_order_book.h"
#include "matching/map_order_book.h"
#include "matching/order_book.h"

static_assert(std::is_abstract_v<OrderBook>);
static_assert(std::is_base_of_v<OrderBook, BitmaskOrderBook>);
static_assert(std::is_base_of_v<OrderBook, MapOrderBook>);
static_assert(std::is_final_v<BitmaskOrderBook>);
static_assert(std::is_final_v<MapOrderBook>);

TEST(OrderBookTest, AddAndFind) {
  BitmaskOrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);
  Order b("ord2", "c2", InstrumentType::ROSE, Side::BUY, 20, 10);

  book.add_order_known_valid(10, &a);
  book.add_order_known_valid(20, &b);

  EXPECT_EQ(book.find_first(), 10);
  EXPECT_EQ(book.find_last(), 20);
}

TEST(OrderBookTest, FIFOPop) {
  BitmaskOrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);
  Order b("ord2", "c2", InstrumentType::ROSE, Side::BUY, 10, 10);

  book.add_order_known_valid(10, &a);
  book.add_order_known_valid(10, &b);

  EXPECT_EQ(book.pop_order_known_valid(10), &a);
  EXPECT_EQ(book.pop_order_known_valid(10), &b);
}

TEST(OrderBookTest, EmptyBookFindsReturnNegative) {
  BitmaskOrderBook book(128);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(OrderBookTest, PopEmptyLevelReturnsNullptr) {
  BitmaskOrderBook book(128);
  EXPECT_EQ(book.pop_order_known_valid(42), nullptr);
}

TEST(OrderBookTest, MaskClearsWhenLevelEmpties) {
  BitmaskOrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);

  book.add_order_known_valid(10, &a);
  EXPECT_EQ(book.find_first(), 10);

  EXPECT_EQ(book.pop_order_known_valid(10), &a);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(OrderBookTest, KnownValidAddAndPopMaintainFifoAndBestLevels) {
  BitmaskOrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 11, 10);
  Order b("ord2", "c2", InstrumentType::ROSE, Side::BUY, 11, 10);
  Order c("ord3", "c3", InstrumentType::ROSE, Side::BUY, 20, 10);

  book.add_order_known_valid(11, &a);
  book.add_order_known_valid(11, &b);
  book.add_order_known_valid(20, &c);

  EXPECT_EQ(book.find_first(), 11);
  EXPECT_EQ(book.find_last(), 20);

  EXPECT_EQ(book.pop_order_known_valid(11), &a);
  EXPECT_EQ(book.pop_order_known_valid(11), &b);
  EXPECT_EQ(book.find_first(), 20);
  EXPECT_EQ(book.find_last(), 20);
  EXPECT_EQ(book.pop_order_known_valid(20), &c);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(OrderBookTest, KnownValidPopReturnsNullptrOnEmptyLevel) {
  BitmaskOrderBook book(64);
  EXPECT_EQ(book.pop_order_known_valid(12), nullptr);
}

TEST(MapOrderBookTest, MaintainsOrderedPriceLevelsAndFifo) {
  MapOrderBook book(128);
  Order first_at_ten("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10,
                     10);
  Order second_at_ten("ord2", "c2", InstrumentType::ROSE, Side::BUY, 10,
                      10);
  Order at_twenty("ord3", "c3", InstrumentType::ROSE, Side::BUY, 20, 10);
  Order at_five("ord4", "c4", InstrumentType::ROSE, Side::BUY, 5, 10);

  OrderBook &strategy = book;
  strategy.add_order_known_valid(10, &first_at_ten);
  strategy.add_order_known_valid(20, &at_twenty);
  strategy.add_order_known_valid(10, &second_at_ten);
  strategy.add_order_known_valid(5, &at_five);

  EXPECT_EQ(strategy.find_first(), 5);
  EXPECT_EQ(strategy.find_last(), 20);
  EXPECT_EQ(strategy.peek_order_known_valid(10), &first_at_ten);
  EXPECT_EQ(strategy.pop_order_known_valid(10), &first_at_ten);
  EXPECT_EQ(strategy.peek_order_known_valid(10), &second_at_ten);
  EXPECT_EQ(strategy.pop_order_known_valid(10), &second_at_ten);
  EXPECT_EQ(strategy.find_first(), 5);
  EXPECT_EQ(strategy.find_last(), 20);
}

TEST(MapOrderBookTest, RemovesEmptyLevels) {
  MapOrderBook book(128);
  Order order("ord1", "c1", InstrumentType::ROSE, Side::SELL, 42, 10);

  book.add_order_known_valid(42, &order);
  EXPECT_EQ(book.pop_order_known_valid(42), &order);
  EXPECT_EQ(book.peek_order_known_valid(42), nullptr);
  EXPECT_EQ(book.pop_order_known_valid(42), nullptr);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(MapOrderBookTest, IgnoresNullOrders) {
  MapOrderBook book(128);

  book.add_order_known_valid(42, nullptr);

  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

#include <gtest/gtest.h>

#include "matching/order_book.h"

TEST(OrderBookTest, AddAndFind) {
  OrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);
  Order b("ord2", "c2", InstrumentType::ROSE, Side::BUY, 20, 10);

  book.add_order_known_valid(10, &a);
  book.add_order_known_valid(20, &b);

  EXPECT_EQ(book.find_first(), 10);
  EXPECT_EQ(book.find_last(), 20);
}

TEST(OrderBookTest, FIFOPop) {
  OrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);
  Order b("ord2", "c2", InstrumentType::ROSE, Side::BUY, 10, 10);

  book.add_order_known_valid(10, &a);
  book.add_order_known_valid(10, &b);

  EXPECT_EQ(book.pop_order_known_valid(10), &a);
  EXPECT_EQ(book.pop_order_known_valid(10), &b);
}

TEST(OrderBookTest, EmptyBookFindsReturnNegative) {
  OrderBook book(128);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(OrderBookTest, PopEmptyLevelReturnsNullptr) {
  OrderBook book(128);
  EXPECT_EQ(book.pop_order_known_valid(42), nullptr);
}

TEST(OrderBookTest, MaskClearsWhenLevelEmpties) {
  OrderBook book(128);
  Order a("ord1", "c1", InstrumentType::ROSE, Side::BUY, 10, 10);

  book.add_order_known_valid(10, &a);
  EXPECT_EQ(book.find_first(), 10);

  EXPECT_EQ(book.pop_order_known_valid(10), &a);
  EXPECT_EQ(book.find_first(), -1);
  EXPECT_EQ(book.find_last(), -1);
}

TEST(OrderBookTest, KnownValidAddAndPopMaintainFifoAndBestLevels) {
  OrderBook book(128);
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
  OrderBook book(64);
  EXPECT_EQ(book.pop_order_known_valid(12), nullptr);
}

#include "../server/map_orderbook.h"
#include "../server/systypes.h"
#include <common/types.h>
#include <gtest/gtest.h>

TEST(MapOrderbookTest, ReturnCorrectInstrumentID) {
    const MapOrderBook map_orderbook(Instrument_id::rose);

    EXPECT_EQ(map_orderbook.instrument_id(), Instrument_id::rose);
}

TEST(MapOrderbookTest, HasBidsTest) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    EXPECT_FALSE(map_orderbook.has_bids());

    constexpr Resting_order order = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    map_orderbook.add_order(Side::buy, order);

    EXPECT_TRUE(map_orderbook.has_bids());
}

TEST(MapOrderbookTest, HasAsksTest) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    EXPECT_FALSE(map_orderbook.has_asks());

    constexpr Resting_order order = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 1000,
    };

    map_orderbook.add_order(Side::sell, order);

    EXPECT_TRUE(map_orderbook.has_asks());
}

TEST(MapOrderbookTest, ReturnCorrectBestBidPrice) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 2000,
    };

    map_orderbook.add_order(Side::buy, order1);
    EXPECT_EQ(map_orderbook.best_bid_price(), order1.price);
    map_orderbook.add_order(Side::buy, order2);
    EXPECT_EQ(map_orderbook.best_bid_price(), order2.price);
}

TEST(MapOrderbookTest, ReturnCorrectBestAskPrice) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 500,
    };

    map_orderbook.add_order(Side::sell, order1);
    EXPECT_EQ(map_orderbook.best_ask_price(), order1.price);
    map_orderbook.add_order(Side::sell, order2);
    EXPECT_EQ(map_orderbook.best_ask_price(), order2.price);
}

TEST(MapOrderbookTest, PeekBestBidOrder) {
    MapOrderBook map_orderbook(Instrument_id::rose);
    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 2000,
    };

    EXPECT_EQ(map_orderbook.peek_best_bid_order(), nullptr);
    map_orderbook.add_order(Side::buy, order1);
    EXPECT_EQ(map_orderbook.peek_best_bid_order()->order_id, order1.order_id);
    map_orderbook.add_order(Side::buy, order2);
    EXPECT_EQ(map_orderbook.peek_best_bid_order()->order_id, order2.order_id);
}

TEST(MapOrderbookTest, PeekBestAskOrder) {
    MapOrderBook map_orderbook(Instrument_id::rose);
    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 500,
    };

    EXPECT_EQ(map_orderbook.peek_best_ask_order(), nullptr);
    map_orderbook.add_order(Side::sell, order1);
    EXPECT_EQ(map_orderbook.peek_best_ask_order()->order_id, order1.order_id);
    map_orderbook.add_order(Side::sell, order2);
    EXPECT_EQ(map_orderbook.peek_best_ask_order()->order_id, order2.order_id);
}

TEST(MapOrderbookTest, PopBestBidOrder) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 2000,
    };

    map_orderbook.add_order(Side::buy, order1);
    map_orderbook.add_order(Side::buy, order2);
    map_orderbook.pop_best_bid_order();

    EXPECT_EQ(map_orderbook.peek_best_bid_order()->order_id, order1.order_id);
}

TEST(MapOrderbookTest, PopBestAskOrder) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::sell,
        .quantity = 100,
        .price = 500,
    };

    map_orderbook.add_order(Side::sell, order1);
    map_orderbook.add_order(Side::sell, order2);
    map_orderbook.pop_best_ask_order();

    EXPECT_EQ(map_orderbook.peek_best_ask_order()->order_id, order1.order_id);
}

TEST(MapOrderbookTest, PriceTimePriority) {
    MapOrderBook map_orderbook(Instrument_id::rose);

    constexpr Resting_order order1 = {
        .order_id = 1000,
        .client_order_id = 1,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    constexpr Resting_order order2 = {
        .order_id = 1001,
        .client_order_id = 2,
        .instrument_id = Instrument_id::rose,
        .book_side = Side::buy,
        .quantity = 100,
        .price = 1000,
    };

    map_orderbook.add_order(Side::buy, order1);
    map_orderbook.add_order(Side::buy, order2);

    EXPECT_EQ(map_orderbook.peek_best_bid_order()->order_id, order1.order_id);

}
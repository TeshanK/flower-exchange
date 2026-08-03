#pragma once

#include "systypes.h"
#include <common/types.h>
#include <concepts>

template <typename B>
concept OrderBook = requires(B book, Side side, Resting_order order) {

    { book.instrument_id() } -> std::convertible_to<Instrument_id>;
    { book.has_bids() } -> std::same_as<bool>;
    { book.has_asks() } -> std::same_as<bool>;

    { book.best_bid_price() } -> std::same_as<Price_ticks>;
    { book.best_ask_price() } -> std::same_as<Price_ticks>;

    { book.peek_best_bid_order() } -> std::same_as<Resting_order*>;
    { book.peek_best_ask_order() } -> std::same_as<Resting_order*>;

    { book.add_order(side, order) };
    { book.pop_best_bid_order() };
    { book.pop_best_ask_order() };
};
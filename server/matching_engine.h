#pragma once

#include "buffer.h"
#include "orderbook.h"
#include "registry.h"
#include <print>
#include <chrono>
#include <algorithm>

template<
    Buffer<Order> InputBuffer,
    Buffer<Report> OutputBuffer,
    OrderBook BookImpl
>
class MatchingEngine {
public:
    explicit MatchingEngine(InputBuffer &input_buffer, OutputBuffer &output_buffer,
                            InstrumentRegistry<BookImpl> &registry)
        : input_buffer_(input_buffer), output_buffer_(output_buffer), registry_(registry) {
    }

    bool step() {
        auto order = input_buffer_.pop();
        if (!order) {
            return false;
        }

        BookImpl *book = registry_.get_book(order->instrument_id);
        if (!book) {
            std::println("Invalid instrument id");
            return true;
        }

        process_order(*book, *order);
        return true;
    }

private:
    void process_order(BookImpl &book, Order &order);

    void match_buy(BookImpl &book, Order &buy_order, Timestamp_ns now);

    void match_sell(BookImpl &book, Order &sell_order, Timestamp_ns now);

    void emit_report(const Report &report) {
        output_buffer_.push(report);
    }

    InputBuffer &input_buffer_;
    OutputBuffer &output_buffer_;
    InstrumentRegistry<BookImpl> &registry_;

    uint32_t server_order_id{1000};
};

template<Buffer<Order> InputBuffer, Buffer<Report> OutputBuffer, OrderBook BookImpl>
void MatchingEngine<InputBuffer, OutputBuffer, BookImpl>::process_order(BookImpl &book, Order &order) {
    order.order_id = server_order_id++;
    const Timestamp_ns now = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    if (order.book_side == Side::buy) {
        match_buy(book, order, now);
    } else {
        match_sell(book, order, now);
    }
}

template<Buffer<Order> InputBuffer, Buffer<Report> OutputBuffer, OrderBook BookImpl>
void MatchingEngine<InputBuffer, OutputBuffer, BookImpl>::match_buy(BookImpl &book, Order &buy_order,
                                                                    const Timestamp_ns now) {
    auto remaining_qty = buy_order.quantity;
    const Quantity original_qty = buy_order.quantity;

    while (book.has_asks() && remaining_qty > 0) {
        if (book.best_ask_price() > buy_order.price) {
            break;
        }

        Resting_order *resting_order = book.peek_best_ask_order();
        if (!resting_order) {
            break;
        }
        const auto fill_qty = std::min(resting_order->quantity, remaining_qty);

        remaining_qty -= fill_qty;
        resting_order->quantity -= fill_qty;
        const auto executed_price = resting_order->price;

        emit_report(make_fill_report(*resting_order, fill_qty, executed_price, now));
        emit_report(make_fill_report(buy_order, fill_qty, remaining_qty, executed_price, now));

        if (resting_order->quantity == 0) {
            book.pop_best_ask_order();
        }
    }

    if (remaining_qty > 0) {
        auto new_resting_order = Resting_order{
            .order_id = buy_order.order_id,
            .client_order_id = buy_order.client_order_id,
            .instrument_id = buy_order.instrument_id,
            .book_side = buy_order.book_side,
            .quantity = remaining_qty,
            .price = buy_order.price,
        };
        if (remaining_qty == original_qty) {
            emit_report(make_newly_created_report(new_resting_order, now));
        }
        book.add_order(Side::buy, new_resting_order);
    }
}

template<Buffer<Order> InputBuffer, Buffer<Report> OutputBuffer, OrderBook BookImpl>
void MatchingEngine<InputBuffer, OutputBuffer, BookImpl>::match_sell(BookImpl &book, Order &sell_order,
                                                                     const Timestamp_ns now) {
    auto remaining_qty = sell_order.quantity;
    const Quantity original_qty = sell_order.quantity;

    while (book.has_bids() && remaining_qty > 0) {
        if (book.best_bid_price() < sell_order.price) {
            break;
        }

        Resting_order *resting_order = book.peek_best_bid_order();
        if (!resting_order) {
            break;
        }
        const auto fill_qty = std::min(resting_order->quantity, remaining_qty);

        remaining_qty -= fill_qty;
        resting_order->quantity -= fill_qty;
        const auto executed_price = resting_order->price;

        emit_report(make_fill_report(*resting_order, fill_qty, executed_price, now));
        emit_report(make_fill_report(sell_order, fill_qty, remaining_qty, executed_price, now));

        if (resting_order->quantity == 0) {
            book.pop_best_bid_order();
        }
    }

    if (remaining_qty > 0) {
        auto new_resting_order = Resting_order{
            .order_id = sell_order.order_id,
            .client_order_id = sell_order.client_order_id,
            .instrument_id = sell_order.instrument_id,
            .book_side = sell_order.book_side,
            .quantity = remaining_qty,
            .price = sell_order.price,
        };
        if (remaining_qty == original_qty) {
            emit_report(make_newly_created_report(new_resting_order, now));
        }
        book.add_order(Side::sell, new_resting_order);
    }
}

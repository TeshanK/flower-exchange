#pragma once

#include <common/types.h>
#include <cstdint>

struct Order {
    Sequence_number sequence_number;
    Order_id order_id{0};
    Client_order_id client_order_id;
    Instrument_id instrument_id;
    Side book_side;
    Quantity quantity;
    Price_ticks price;
    Timestamp_ns ingress_timestamp;
};

struct Resting_order {
    Order_id order_id{0};
    Client_order_id client_order_id;
    Instrument_id instrument_id;
    Side book_side;
    Quantity quantity;
    Price_ticks price;
};

struct Report {
    Order_id order_id;
    Client_order_id client_order_id;
    Instrument_id instrument_id;
    Side book_side;
    Quantity quantity;
    Price_ticks price;
    Execution_status status; // Status of the order execution
    Rejection_reason reject_reason{0}; // Reason for rejection if the order was rejected
    Timestamp_ns execution_timestamp{0}; // Nanoseconds since epoch when the execution report was generated
};

inline Report make_fill_report(const Resting_order &resting_order, const Quantity filled_qty, const Price_ticks executed_price,
                               const Timestamp_ns now) {
    return Report{
        .order_id = resting_order.order_id,
        .client_order_id = resting_order.client_order_id,
        .instrument_id = resting_order.instrument_id,
        .book_side = resting_order.book_side,
        .quantity = filled_qty,
        .price = executed_price,
        .status = (resting_order.quantity == 0) ? Execution_status::fully_filled : Execution_status::partially_filled,
        .execution_timestamp = now,
    };
}

inline Report make_fill_report(const Order& order, const Quantity filled_qty, const Quantity remaining_qty, const Price_ticks executed_price, const Timestamp_ns now) {
    return Report{
        .order_id = order.order_id,
        .client_order_id = order.client_order_id,
        .instrument_id = order.instrument_id,
        .book_side = order.book_side,
        .quantity = filled_qty,
        .price = executed_price,
        .status = (remaining_qty == 0) ? Execution_status::fully_filled : Execution_status::partially_filled,
        .execution_timestamp = now,
    };
}

inline Report make_newly_created_report(const Resting_order& order, const Timestamp_ns now) {
    return Report{
        .order_id = order.order_id,
        .client_order_id = order.client_order_id,
        .instrument_id = order.instrument_id,
        .book_side = order.book_side,
        .quantity = order.quantity,
        .price = order.price,
        .status = Execution_status::newly_created,
        .execution_timestamp = now,
    };
}

#pragma once

#include <cstdint>

enum class Instrument_id : uint8_t {
    rose = 1,
    lavender = 2,
    lotus = 3,
    tulip = 4,
    orchid = 5,
    invalid = 255,
};

enum class Side : uint8_t {
    buy = 1,
    sell = 2,
};

enum class Execution_status: uint8_t {
    newly_created = 1,
    rejected = 2,
    partially_filled = 3,
    fully_filled = 4,
};

enum class Rejection_reason: uint8_t {
    none = 0,
    invalid_instrument = 1,
    invalid_side = 2,
    invalid_quantity = 3,
    invalid_price = 4,
};

using Client_order_id = uint32_t;
using Order_id = uint64_t;
using Quantity = uint16_t;
using Price_ticks = uint32_t;
using Timestamp_ns = uint64_t;
using Sequence_number = uint64_t;
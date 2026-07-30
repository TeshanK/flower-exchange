#pragma once

#include <cstdint>

// SEP (Simple Exchange Protocol) message definitions

#pragma pack(push, 1) // Ensure no padding is added to the structures

enum class Message_type : uint8_t {
    new_order = 1,
    execution_report = 2,
    cancel_order = 3,
};

enum class Instrument_id : uint8_t {
    rose = 1,
    lavender = 2,
    lotus = 3,
    tulip = 4,
    orchid = 5,
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

using Price_ticks = uint32_t;

// Message header structure for both New_order and Execution_report messages.
// Current version: 1
struct Message_header {
    uint8_t version{1};         // Protocol version
    Message_type message_type;  // Type of the message (e.g., new_order, execution_report, cancel_order)
    uint16_t body_length;       // Length of the message body in bytes (excluding the header)
};

// New_order payload structure
struct New_order {
    Message_header header;
    uint32_t client_order_id;       // Unique identifier for the order assigned by the client
    Instrument_id instrument_id;    // Identifier for the instrument being traded
    Side book_side;                 // Buy or sell side
    uint16_t quantity;              // Quantity of the order
    Price_ticks price;              // Price of the order as price ticks
};

// Execution_report payload structure
struct Execution_report {
    Message_header header;
    uint64_t timestamp;             // Nanoseconds since epoch when the execution report was generated
    uint32_t order_id;              // Unique identifier for the order assigned by the server
    uint32_t client_order_id;       // Unique identifier for the order assigned by the client
    Instrument_id instrument_id;    // Identifier for the instrument being traded
    Side book_side;                 // Buy or sell side
    uint16_t quantity;              // Quantity of the order
    Price_ticks price;              // Price of the order as price ticks
    Execution_status status;        // Status of the order execution
    Rejection_reason reject_reason; // Reason for rejection if the order was rejected
    uint16_t reserved{0};               // Reserved for future use, should be set to 0
};

#pragma pack(pop) // Restore the previous packing alignment

// Compile time asserts to guarantee the layout sizes
static_assert(sizeof(Message_header) == 4, "Message_header size must be 4 bytes");
static_assert(sizeof(New_order) == 16, "New_order size must be 16 bytes");
static_assert(sizeof(Execution_report) == 32, "Execution_report size must be 32 bytes");
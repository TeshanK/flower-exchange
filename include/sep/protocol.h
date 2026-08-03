// SEP (Simple Exchange Protocol) message definitions

#pragma once

#include <cstdint>
#include <common/types.h>

#pragma pack(push, 1) // Ensure no padding is added to the structures

enum class Message_type : uint8_t {
    new_order = 1,
    execution_report = 2,
};

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
    Client_order_id client_order_id;       // Unique identifier for the order assigned by the client
    Instrument_id instrument_id;    // Identifier for the instrument being traded
    Side book_side;                 // Buy or sell side
    Quantity quantity;              // Quantity of the order
    Price_ticks price;              // Price of the order as price ticks
};

// Execution_report payload structure
struct Execution_report {
    Message_header header;
    Timestamp_ns timestamp;             // Nanoseconds since epoch when the execution report was generated
    Order_id order_id;              // Unique identifier for the order assigned by the server
    Client_order_id client_order_id;       // Unique identifier for the order assigned by the client
    Instrument_id instrument_id;    // Identifier for the instrument being traded
    Side book_side;                 // Buy or sell side
    Quantity quantity;              // Quantity of the order
    Price_ticks price;              // Price of the order as price ticks
    Execution_status status;        // Status of the order execution
    Rejection_reason reject_reason; // Reason for rejection if the order was rejected
    uint16_t reserved{0};               // Reserved for future use, should be set to 0
};

#pragma pack(pop) // Restore the previous packing alignment

// Compile time asserts to guarantee the layout sizes
static_assert(sizeof(Message_header) == 4, "Message_header size must be 4 bytes");
static_assert(sizeof(New_order) == 16, "New_order size must be 16 bytes");
static_assert(sizeof(Execution_report) == 36, "Execution_report size must be 32 bytes");
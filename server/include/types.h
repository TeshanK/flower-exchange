#pragma once

#include <sep/protocol.h>
#include <cstdint>

struct Order
{
    New_order raw_order;
    uint64_t sequence_number;
    uint64_t ingress_timestamp; // Nanoseconds since epoch at ingress
};
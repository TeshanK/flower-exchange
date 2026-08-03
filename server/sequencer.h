#pragma once

#include "systypes.h"
#include <sep/protocol.h>
#include <cstdint>
#include <chrono>

class Sequencer
{
public:
    Sequencer() = default;

    [[nodiscard]] Order sequence(const New_order& raw_order) noexcept {
        const uint64_t seq = next_sequence_number_++;
        const uint64_t now = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        return Order {
            .sequence_number = seq,
            .client_order_id = raw_order.client_order_id,
            .instrument_id = raw_order.instrument_id,
            .book_side = raw_order.book_side,
            .quantity = raw_order.quantity,
            .price = raw_order.price,
            .ingress_timestamp = now,
        };
    }

private:
    uint64_t next_sequence_number_{1};
};
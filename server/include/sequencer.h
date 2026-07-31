#pragma once

#include <cstdint>
#include <types.h>
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
            .raw_order = raw_order,
            .sequence_number = seq,
            .ingress_timestamp = now,
        };
    }

private:
    uint64_t next_sequence_number_{1};
};
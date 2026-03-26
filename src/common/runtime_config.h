#pragma once

#include <cstddef>
#include <cstdint>

// Centralized compile-time configuration.
struct RuntimeConfig final {
    static constexpr double kTickSize = 0.01;

    static constexpr int kQuantityMultiple = 10;
    static constexpr int kMinQuantity = 10;
    static constexpr int kMaxQuantity = 1000;

    static constexpr double kMinValidPrice = 0.0;
    static constexpr std::size_t kOrderBookTickCapacity = 100001;
    static constexpr std::uint64_t kMaxValidTick = kOrderBookTickCapacity - 1;
    static constexpr double kMaxValidPrice =
        static_cast<double>(kMaxValidTick) * kTickSize;

    static constexpr std::size_t kOrderPoolInitialCapacity = 65536;
    static constexpr std::size_t kReportPoolInitialCapacity = 8192;

    static constexpr std::size_t kInboundQueueCapacity = 1 << 16;
    static constexpr std::size_t kOutboundQueueCapacity = 1 << 14;
};
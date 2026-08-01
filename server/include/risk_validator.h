#pragma once

#include <sep/protocol.h>
#include <types.h>
#include <expected>

class RiskValidator
{
public:
    // Validates business rules, price bands, and quantity limits
    [[nodiscard]] static auto validate(const Order &order)
        -> std::expected<void, Rejection_reason>
    {
        const auto &req = order.raw_order;

        // 1. Sanity check quantity
        if (req.quantity == 0 || req.quantity > MAX_ORDER_QUANTITY)
        {
            return std::unexpected(Rejection_reason::invalid_quantity);
        }

        // 2. Sanity check price
        if (req.price == 0 || req.price > MAX_ORDER_PRICE)
        {
            return std::unexpected(Rejection_reason::invalid_price);
        }

        // 3. Instrument validation
        if (static_cast<uint8_t>(req.instrument_id) < 1 ||
            static_cast<uint8_t>(req.instrument_id) > 5)
        {
            return std::unexpected(Rejection_reason::invalid_instrument);
        }

        // 4. Side validation
        if (req.book_side != Side::buy && req.book_side != Side::sell)
        {
            return std::unexpected(Rejection_reason::invalid_side);
        }

        return {}; // Order is valid!
    }

private:
    static constexpr uint16_t MAX_ORDER_QUANTITY = 10'000;
    static constexpr Price_ticks MAX_ORDER_PRICE = 1'000'000;
};
#pragma once

#include "common/types.h"

#include <cmath>
#include <string_view>
#include <utility>

/*
* Validates a parsed order and returns {ok, reason}.
*
* @instrument: parsed instrument type
* @side: parsed side (int representation of Side enum)
* @price: parsed price as double
* @quantity: parsed quantity as int
*
* On success returns {true, nullptr}; on failure returns a static reason string.
*/
inline std::pair<bool, const char*> validate_order(InstrumentType instrument,
                                                   int side,
                                                   double price,
                                                   int quantity) {
    if (instrument == InstrumentType::COUNT) {
        return {false, "Invalid instrument"};
    }

    if (side != static_cast<int>(Side::BUY) &&
        side != static_cast<int>(Side::SELL)) {
        return {false, "Invalid side"};
    }

    if (quantity % QUANTITY_MULTIPLE != 0 || quantity < MIN_QUANTITY ||
        quantity > MAX_QUANTITY) {
        return {false, "Invalid size"};
    }

    if (!std::isfinite(price) || price <= MIN_VALID_PRICE || price > MAX_VALID_PRICE) {
        return {false, "Invalid price"};
    }

    return {true, nullptr};
}

inline std::pair<bool, const char*> validate_order(std::string_view instrument,
                                                   int side,
                                                   double price,
                                                   int quantity) {
    InstrumentType parsed = InstrumentType::COUNT;
    parse_instrument(instrument, parsed);
    return validate_order(parsed, side, price, quantity);
}

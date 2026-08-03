#pragma once

#include "orderbook.h"
#include <common/types.h>
#include <array>

template <OrderBook BookImpl, std::size_t Num_instruments = 5>
class InstrumentRegistry
{
public:
    InstrumentRegistry()
    : books_{
        BookImpl{Instrument_id::rose},
        BookImpl{Instrument_id::lavender},
        BookImpl{Instrument_id::lotus},
        BookImpl{Instrument_id::tulip},
        BookImpl{Instrument_id::orchid}
    } {}

    [[nodiscard]] BookImpl* get_book(Instrument_id id) noexcept {
        const auto raw = static_cast<std::size_t>(id);
        if (raw == 0 || raw > Num_instruments) {
            return nullptr;
        }
        return &books_[raw - 1];
    }

private:
    std::array<BookImpl, Num_instruments> books_;
};
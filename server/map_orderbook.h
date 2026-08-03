#pragma once

#include <common/types.h>
#include <map>
#include <deque>

#include "systypes.h"

class MapOrderBook {
public:
    explicit MapOrderBook(const Instrument_id instrument_id)
        : instrument_id_(instrument_id) {}

    [[nodiscard]] Instrument_id instrument_id() const noexcept { return instrument_id_; }

    [[nodiscard]] bool has_bids() const noexcept { return !bids_.empty(); }
    [[nodiscard]] bool has_asks() const noexcept { return !asks_.empty(); }

    [[nodiscard]] Price_ticks best_bid_price() const noexcept { return bids_.begin()->first; }
    [[nodiscard]] Price_ticks best_ask_price() const noexcept { return asks_.begin()->first; }

    Resting_order* peek_best_bid_order() {
        return bids_.empty() ? nullptr : &bids_.begin()->second.front();
    }

    Resting_order* peek_best_ask_order() {
        return asks_.empty() ? nullptr : &asks_.begin()->second.front();
    }

    void add_order(const Side side, Resting_order order) {
        if (side == Side::buy) {
            bids_[order.price].push_back(order);
        } else {
            asks_[order.price].push_back(order);
        }
    }

    void pop_best_bid_order() {
        if (bids_.empty()) {
            return;
        }
        const auto it = bids_.begin();
        it->second.pop_front();
        if (it->second.empty()) {
            bids_.erase(it);
        }
    }

    void pop_best_ask_order() {
        if (asks_.empty()) {
            return;
        }
        const auto it = asks_.begin();
        it->second.pop_front();
        if (it->second.empty()) {
            asks_.erase(it);
        }
    }


private:
    Instrument_id instrument_id_;

    std::map<Price_ticks, std::deque<Resting_order>, std::greater<Price_ticks>> bids_;
    std::map<Price_ticks, std::deque<Resting_order>, std::less<Price_ticks>> asks_;
};

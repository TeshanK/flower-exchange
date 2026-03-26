#pragma once

#include "common/runtime_config.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <ctime>

using PriceTick = uint64_t;

constexpr double TICK_SIZE = RuntimeConfig::kTickSize;

enum class Side : uint8_t {
    BUY = 1,
    SELL = 2,
};

enum class InstrumentType : uint8_t {
    ROSE = 0,
    LAVENDER = 1,
    LOTUS = 2,
    TULIP = 3,
    ORCHID = 4,
    COUNT,
};

enum class ExecStatus : uint8_t {
    NEW = 0,
    REJECTED = 1,
    FILL = 2,
    PFILL = 3,
};

constexpr int QUANTITY_MULTIPLE = RuntimeConfig::kQuantityMultiple;
constexpr int MIN_QUANTITY = RuntimeConfig::kMinQuantity;
constexpr int MAX_QUANTITY = RuntimeConfig::kMaxQuantity;
constexpr double MIN_VALID_PRICE = RuntimeConfig::kMinValidPrice;
constexpr std::size_t ORDER_BOOK_TICK_CAPACITY = RuntimeConfig::kOrderBookTickCapacity;
constexpr PriceTick MAX_VALID_TICK = static_cast<PriceTick>(RuntimeConfig::kMaxValidTick);
constexpr double MAX_VALID_PRICE = RuntimeConfig::kMaxValidPrice;

// Instrument names used for parsing and output text.
extern const std::array<const char*, 5> kValidInstrumentNames;

// Converts decimal price to ticks
PriceTick double_to_ticks(double price);
// Converts ticks back to decimal
double ticks_to_double(PriceTick ticks);

// Enum-to-text helpers used by report formatting paths
const char* instrument_to_string(InstrumentType instrument);
const char* exec_status_to_string(ExecStatus status);

// Parses instrument text into enum value
bool parse_instrument(std::string_view instrument, InstrumentType& out);

// Numeric parsing helpers for CSV fields.
int string_view_to_int(std::string_view sv);
double string_view_to_double(std::string_view sv);

// Formats decimal/tick prices into fixed two-decimal string buffers.
std::size_t format_price_to_buffer(double price, char* buffer, std::size_t buffer_size);
std::size_t format_ticks_to_buffer(PriceTick ticks, char* buffer, std::size_t buffer_size);

// Intrusive order-book node
struct Order {
    char oid[32];               // order id given by the exchange
    char coid[64];              // client order id given by the client
    InstrumentType instrument;  // instrument type
    Side side;                  // buy or sell
    PriceTick price;            // price in ticks
    uint16_t quantity;          // quantity in multiples of QUANTITY_MULTIPLE
    Order* next;                // pointer to the next order of the same price level

    // Creates empty order with sentinel instrument and zero quantity.
    Order()
        : oid{0}, coid{0}, instrument(InstrumentType::COUNT), side(Side::BUY),
          price(0), quantity(0), next(nullptr) {}

    // Creates order and copies text ids into buffers.
    Order(const char* oid, const char* coid, InstrumentType instrument, Side side,
          PriceTick price, uint16_t quantity)
        : oid{0}, coid{0}, instrument(instrument), side(side), price(price),
          quantity(quantity), next(nullptr) {
        if (oid) {
            int i = 0;
            for (; i < 31 && oid[i] != '\0'; ++i) {
                this->oid[i] = oid[i];
            }
            this->oid[i] = '\0';
        }
        if (coid) {
            int i = 0;
            for (; i < 63 && coid[i] != '\0'; ++i) {
                this->coid[i] = coid[i];
            }
            this->coid[i] = '\0';
        }
    }
};

struct ExecutionReport {
    char oid[32];               // order id given by the exchange
    char coid[64];              // client order id given by the client
    InstrumentType instrument;  // instrument type
    Side side;                  // buy or sell
    PriceTick price;            // price in ticks
    uint16_t quantity;          // quantity in multiples of QUANTITY_MULTIPLE
    ExecStatus status;          // execution status
    char reason[50];            // optional reason text for rejections
    char timestamp[20];         // execution timestamp in "YYYYMMDD-HHMMSS.sss" format

    static inline void fill_current_timestamp(char* out, std::size_t out_size) {
        if (!out || out_size == 0) {
            return;
        }
        if (out_size < 20) {
            out[0] = '\0';
            return;
        }

        const auto now = std::chrono::system_clock::now();
        const auto epoch_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch());
        const int millis = static_cast<int>(epoch_ms.count() % 1000);

        const std::time_t tt = std::chrono::system_clock::to_time_t(now);
        std::tm tm_local{};
        localtime_r(&tt, &tm_local);

        const int written = std::snprintf(
            out,
            out_size,
            "%04d%02d%02d-%02d%02d%02d.%03d",
            tm_local.tm_year + 1900,
            tm_local.tm_mon + 1,
            tm_local.tm_mday,
            tm_local.tm_hour,
            tm_local.tm_min,
            tm_local.tm_sec,
            millis);

        if (written < 0 || static_cast<std::size_t>(written) >= out_size) {
            out[0] = '\0';
        }
    }

    // Creates empty report in NEW status.
    ExecutionReport()
                : oid{0}, coid{0}, instrument(InstrumentType::COUNT), side(Side::BUY),
                    price(0), quantity(0), status(ExecStatus::NEW),
                    reason{0}, timestamp{0} {
                fill_current_timestamp(timestamp, sizeof(timestamp));
        }

    // Creates report and copies id/reason text into buffers.
    ExecutionReport(const char* oid, const char* coid, InstrumentType instrument,
                    Side side, PriceTick price, uint16_t quantity,
                                        ExecStatus status, const char* reason_text = "",
                                        const char* timestamp_text = nullptr)
        : oid{0}, coid{0}, instrument(instrument), side(side), price(price),
          quantity(quantity), status(status), reason{0}, timestamp{0} {
        if (oid) {
            int i = 0;
            for (; i < 31 && oid[i] != '\0'; ++i) {
                this->oid[i] = oid[i];
            }
            this->oid[i] = '\0';
        }
        if (coid) {
            int i = 0;
            for (; i < 63 && coid[i] != '\0'; ++i) {
                this->coid[i] = coid[i];
            }
            this->coid[i] = '\0';
        }
        if (reason_text) {
            int i = 0;
            // cppcheck-suppress arrayIndexThenCheck
            for (; reason_text[i] != '\0' && i < 49; ++i) {
                reason[i] = reason_text[i];
            }
            reason[i] = '\0';
        }
        if (timestamp_text && timestamp_text[0] != '\0') {
            int i = 0;
            for (; i < 19 && timestamp_text[i] != '\0'; ++i) {
                timestamp[i] = timestamp_text[i];
            }
            timestamp[i] = '\0';
        } else {
            fill_current_timestamp(timestamp, sizeof(timestamp));
        }
    }
};

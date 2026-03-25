#include "common/types.h"

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>

TEST(TypesTest, TickRoundTripIsStableAcrossRange) {
    for (PriceTick tick = 1; tick <= 10000; ++tick) {
        const double price = ticks_to_double(tick);
        const PriceTick round_trip = double_to_ticks(price);
        EXPECT_EQ(round_trip, tick) << "tick=" << tick;
    }
}

TEST(TypesTest, StringViewToDoubleParsesWhitespaceAndScientificNotation) {
    EXPECT_DOUBLE_EQ(string_view_to_double(" 45.55 "), 45.55);
    EXPECT_DOUBLE_EQ(string_view_to_double("1e2"), 100.0);
    EXPECT_DOUBLE_EQ(string_view_to_double("1.25e-1"), 0.125);
    EXPECT_DOUBLE_EQ(string_view_to_double("+0.50"), 0.50);
    EXPECT_DOUBLE_EQ(string_view_to_double("-2.5"), -2.5);
}

TEST(TypesTest, StringViewToDoubleRejectsInvalidForms) {
    EXPECT_THROW(string_view_to_double(""), std::invalid_argument);
    EXPECT_THROW(string_view_to_double("abc"), std::invalid_argument);
    EXPECT_THROW(string_view_to_double("12.3x"), std::invalid_argument);
    EXPECT_THROW(string_view_to_double("1e"), std::invalid_argument);
}

TEST(TypesTest, StringViewToIntParsesAndRejectsInvalidForms) {
    EXPECT_EQ(string_view_to_int(" 100 "), 100);
    EXPECT_EQ(string_view_to_int("+42"), 42);
    EXPECT_EQ(string_view_to_int("-7"), -7);
    EXPECT_THROW(string_view_to_int(""), std::invalid_argument);
    EXPECT_THROW(string_view_to_int("10x"), std::invalid_argument);
}

TEST(TypesTest, StringViewToIntClampsInsteadOfWrappingOnHugeInput) {
    EXPECT_EQ(string_view_to_int("999999999999999999"), std::numeric_limits<int>::max());
    EXPECT_EQ(string_view_to_int("-999999999999999999"), -std::numeric_limits<int>::max());
}

TEST(TypesTest, StringViewToDoubleHugeExponentDoesNotOverflowIntAccumulator) {
    const double parsed = string_view_to_double("1e999999999");
    EXPECT_TRUE(std::isinf(parsed));
}

TEST(TypesTest, DoubleToTicksHandlesNonFiniteInputSafely) {
    EXPECT_EQ(double_to_ticks(std::numeric_limits<double>::infinity()),
              std::numeric_limits<PriceTick>::max());
    EXPECT_EQ(double_to_ticks(std::numeric_limits<double>::quiet_NaN()),
              std::numeric_limits<PriceTick>::max());
}

TEST(TypesTest, FormatPriceToBufferAlwaysTwoDecimals) {
    std::array<char, 32> buf{};

    format_price_to_buffer(0.01, buf.data(), buf.size());
    EXPECT_STREQ(buf.data(), "0.01");

    format_price_to_buffer(10.5, buf.data(), buf.size());
    EXPECT_STREQ(buf.data(), "10.50");

    format_price_to_buffer(999.99, buf.data(), buf.size());
    EXPECT_STREQ(buf.data(), "999.99");

    format_price_to_buffer(100.0, buf.data(), buf.size());
    EXPECT_STREQ(buf.data(), "100.00");
}

TEST(TypesTest, SymbolStringMappingsAreStable) {
    EXPECT_STREQ(exec_status_to_string(ExecStatus::NEW), "New");
    EXPECT_STREQ(exec_status_to_string(ExecStatus::REJECTED), "Reject");
    EXPECT_STREQ(exec_status_to_string(ExecStatus::FILL), "Fill");
    EXPECT_STREQ(exec_status_to_string(ExecStatus::PFILL), "Pfill");
    EXPECT_STREQ(instrument_to_string(InstrumentType::ROSE), "Rose");
}

TEST(TypesTest, ParseInstrumentAcceptsAllCanonicalNames) {
    InstrumentType parsed = InstrumentType::COUNT;

    EXPECT_TRUE(parse_instrument("Rose", parsed));
    EXPECT_EQ(parsed, InstrumentType::ROSE);

    EXPECT_TRUE(parse_instrument("Lavender", parsed));
    EXPECT_EQ(parsed, InstrumentType::LAVENDER);

    EXPECT_TRUE(parse_instrument("Lotus", parsed));
    EXPECT_EQ(parsed, InstrumentType::LOTUS);

    EXPECT_TRUE(parse_instrument("Tulip", parsed));
    EXPECT_EQ(parsed, InstrumentType::TULIP);

    EXPECT_TRUE(parse_instrument("Orchid", parsed));
    EXPECT_EQ(parsed, InstrumentType::ORCHID);

    EXPECT_FALSE(parse_instrument("Invalid", parsed));
}

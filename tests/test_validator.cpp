#include "common/validator.h"

#include <gtest/gtest.h>

#include <limits>

TEST(ValidatorTest, AcceptsValidOrder) {
    auto [ok, reason] = validate_order("Rose", 1, 1.0, 100);
    EXPECT_TRUE(ok);
    EXPECT_EQ(reason, nullptr);
}

TEST(ValidatorTest, RejectsInvalidValues) {
    EXPECT_FALSE(validate_order("", 1, 1.0, 100).first);
    EXPECT_FALSE(validate_order("Rose", 3, 1.0, 100).first);
    EXPECT_FALSE(validate_order("Rose", 1, -1.0, 100).first);
    EXPECT_FALSE(validate_order("Rose", 1, 1.0, 15).first);
}

TEST(ValidatorTest, ReturnsInvalidSizeForBadQuantity) {
    auto [ok, reason] = validate_order("Rose", 1, 1.0, 15);
    EXPECT_FALSE(ok);
    EXPECT_STREQ(reason, "Invalid size");
}

TEST(ValidatorTest, ReturnsInvalidPriceWhenQuantityValid) {
    auto [ok, reason] = validate_order("Rose", 1, -1.0, 100);
    EXPECT_FALSE(ok);
    EXPECT_STREQ(reason, "Invalid price");
}

TEST(ValidatorTest, AcceptsAllSupportedInstruments) {
    EXPECT_TRUE(validate_order("Rose", 1, 10.0, 100).first);
    EXPECT_TRUE(validate_order("Lavender", 2, 10.0, 100).first);
    EXPECT_TRUE(validate_order("Lotus", 1, 10.0, 100).first);
    EXPECT_TRUE(validate_order("Tulip", 2, 10.0, 100).first);
    EXPECT_TRUE(validate_order("Orchid", 1, 10.0, 100).first);
}

TEST(ValidatorTest, QuantityBoundaries) {
    EXPECT_TRUE(validate_order("Rose", 1, 1.0, 10).first);
    EXPECT_TRUE(validate_order("Rose", 2, 1.0, 1000).first);

    EXPECT_FALSE(validate_order("Rose", 1, 1.0, 0).first);
    EXPECT_FALSE(validate_order("Rose", 1, 1.0, 5).first);
    EXPECT_FALSE(validate_order("Rose", 1, 1.0, 1001).first);
}

TEST(ValidatorTest, SideBoundaries) {
    EXPECT_FALSE(validate_order("Rose", 0, 1.0, 100).first);
    EXPECT_FALSE(validate_order("Rose", 3, 1.0, 100).first);
    EXPECT_TRUE(validate_order("Rose", 1, 1.0, 100).first);
    EXPECT_TRUE(validate_order("Rose", 2, 1.0, 100).first);
}

TEST(ValidatorTest, PriceBoundaries) {
    EXPECT_TRUE(validate_order("Rose", 1, MAX_VALID_PRICE, 100).first);
    EXPECT_FALSE(validate_order("Rose", 1, MAX_VALID_PRICE + TICK_SIZE, 100).first);
}

TEST(ValidatorTest, RejectsNonFinitePrices) {
    EXPECT_FALSE(validate_order("Rose", 1, std::numeric_limits<double>::infinity(), 100).first);
    EXPECT_FALSE(validate_order("Rose", 1, -std::numeric_limits<double>::infinity(), 100).first);
    EXPECT_FALSE(validate_order("Rose", 1, std::numeric_limits<double>::quiet_NaN(), 100).first);
}

TEST(ValidatorTest, RejectionReasonsAreExactForEveryCategory) {
    {
        auto [ok, reason] = validate_order("", 1, 1.0, 100);
        EXPECT_FALSE(ok);
        EXPECT_STREQ(reason, "Invalid instrument");
    }

    {
        auto [ok, reason] = validate_order("Rose", 3, 1.0, 100);
        EXPECT_FALSE(ok);
        EXPECT_STREQ(reason, "Invalid side");
    }

    {
        auto [ok, reason] = validate_order("Rose", 1, -1.0, 100);
        EXPECT_FALSE(ok);
        EXPECT_STREQ(reason, "Invalid price");
    }

    {
        auto [ok, reason] = validate_order("Rose", 1, 1.0, 15);
        EXPECT_FALSE(ok);
        EXPECT_STREQ(reason, "Invalid size");
    }
}

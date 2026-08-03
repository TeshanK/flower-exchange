#include "../server/matching_engine.h"
#include "../server/stl_queue.h"
#include "../server/systypes.h"
#include "../server/registry.h"
#include "../server/map_orderbook.h"
#include <common/types.h>
#include <gtest/gtest.h>

namespace {
    class MatchingEngineTest : public ::testing::Test {
    protected:
        MatchingEngineTest()
            : matching_engine_(input_buffer_, output_buffer_, registry_) {
        }

        STLQueue<Order> input_buffer_;
        STLQueue<Report> output_buffer_;
        InstrumentRegistry<MapOrderBook> registry_;

        MatchingEngine<STLQueue<Order>, STLQueue<Report>, MapOrderBook> matching_engine_;
    };

    constexpr Sequence_number starting_seq_num = 1;
    constexpr Order_id starting_oid = 1000;
    constexpr Client_order_id starting_coid = 1;

    Sequence_number curr_seq = starting_seq_num;
    Order_id curr_oid = starting_oid;
    Client_order_id curr_coid = starting_coid;

    Order create_order(
        Instrument_id inst,
        Side side,
        Quantity qty,
        Price_ticks price) {
        return Order{
            .sequence_number = curr_seq++,
            .order_id = curr_oid++,
            .client_order_id = curr_coid++,
            .instrument_id = inst,
            .book_side = side,
            .quantity = qty,
            .price = price,
            .ingress_timestamp = 0
        };
    }
}

TEST_F(MatchingEngineTest, EmptyInputBuffer) {
    EXPECT_FALSE(matching_engine_.step());
}

TEST_F(MatchingEngineTest, EmptyInputBufferReturnFalse) {
    EXPECT_FALSE(matching_engine_.step());
}

TEST_F(MatchingEngineTest, ValidOrderReturnTrue) {
    Order order = create_order(Instrument_id::rose, Side::buy, 100, 1000);

    input_buffer_.push(order);

    EXPECT_TRUE(matching_engine_.step());
}

TEST_F(MatchingEngineTest, InvalidOrderReturnTrueNoReport) {
    Order order = create_order(Instrument_id::invalid, Side::buy, 100, 1000);

    input_buffer_.push(order);
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_FALSE(output_buffer_.top().has_value());
}

TEST_F(MatchingEngineTest, NewRestingOrder) {
    Order order = create_order(Instrument_id::rose, Side::buy, 100, 1000);

    input_buffer_.push(order);
    EXPECT_TRUE(matching_engine_.step());

    const auto val = output_buffer_.pop();
    EXPECT_EQ(val->order_id, order.order_id);
    EXPECT_EQ(val->status, Execution_status::newly_created);
}

TEST_F(MatchingEngineTest, ExactFill) {
    Order order1 = create_order(Instrument_id::rose, Side::sell, 100, 1000);

    Order order2 = create_order(Instrument_id::rose, Side::buy, 100, 1000);

    input_buffer_.push(order1);
    input_buffer_.push(order2);
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());

    const auto val1 = output_buffer_.pop();
    EXPECT_EQ(val1->order_id, order1.order_id);
    EXPECT_EQ(val1->status, Execution_status::newly_created);

    const auto val2 = output_buffer_.pop();
    EXPECT_EQ(val2->order_id, order1.order_id);
    EXPECT_EQ(val2->status, Execution_status::fully_filled);

    const auto val3 = output_buffer_.pop();
    EXPECT_EQ(val3->order_id, order2.order_id);
    EXPECT_EQ(val3->status, Execution_status::fully_filled);

    EXPECT_FALSE(output_buffer_.top().has_value());
}

TEST_F(MatchingEngineTest, PartialFillAggressive) {
    Order order1 = create_order(Instrument_id::rose, Side::sell, 100, 1000);

    Order order2 = create_order(Instrument_id::rose, Side::buy, 250, 1000);

    input_buffer_.push(order1);
    input_buffer_.push(order2);
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());

    const auto val1 = output_buffer_.pop();
    EXPECT_EQ(val1->order_id, order1.order_id);
    EXPECT_EQ(val1->status, Execution_status::newly_created);

    const auto val2 = output_buffer_.pop();
    EXPECT_EQ(val2->order_id, order1.order_id);
    EXPECT_EQ(val2->status, Execution_status::fully_filled);

    const auto val3 = output_buffer_.pop();
    EXPECT_EQ(val3->order_id, order2.order_id);
    EXPECT_EQ(val3->status, Execution_status::partially_filled);
    EXPECT_EQ(val3->quantity, order1.quantity);

    EXPECT_FALSE(output_buffer_.top().has_value());
}

TEST_F(MatchingEngineTest, PartialFillPassive) {
    Order order1 = create_order(Instrument_id::rose, Side::sell, 250, 1000);

    Order order2 = create_order(Instrument_id::rose, Side::buy, 100, 1000);

    input_buffer_.push(order1);
    input_buffer_.push(order2);
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());

    const auto val1 = output_buffer_.pop();
    EXPECT_EQ(val1->order_id, order1.order_id);
    EXPECT_EQ(val1->status, Execution_status::newly_created);

    const auto val2 = output_buffer_.pop();
    EXPECT_EQ(val2->order_id, order1.order_id);
    EXPECT_EQ(val2->status, Execution_status::partially_filled);

    const auto val3 = output_buffer_.pop();
    EXPECT_EQ(val3->order_id, order2.order_id);
    EXPECT_EQ(val3->status, Execution_status::fully_filled);
    EXPECT_EQ(val3->quantity, order2.quantity);

    EXPECT_FALSE(output_buffer_.top().has_value());
}

TEST_F(MatchingEngineTest, MultipleFills) {
    Order order1 = create_order(Instrument_id::rose, Side::sell, 50, 1000);

    Order order2 = create_order(Instrument_id::rose, Side::sell, 70, 1000);

    Order order3 = create_order(Instrument_id::rose, Side::sell, 80, 1000);

    Order order4 = create_order(Instrument_id::rose, Side::buy, 200, 1000);

    input_buffer_.push(order1);
    input_buffer_.push(order2);
    input_buffer_.push(order3);
    input_buffer_.push(order4);

    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());

    auto val1 = output_buffer_.pop();
    EXPECT_EQ(val1->order_id, order1.order_id);
    EXPECT_EQ(val1->status, Execution_status::newly_created);

    const auto val2 = output_buffer_.pop();
    EXPECT_EQ(val2->order_id, order2.order_id);
    EXPECT_EQ(val2->status, Execution_status::newly_created);

    const auto val3 = output_buffer_.pop();
    EXPECT_EQ(val3->order_id, order3.order_id);
    EXPECT_EQ(val3->status, Execution_status::newly_created);

    const auto val4 = output_buffer_.pop();
    EXPECT_EQ(val4->order_id, order1.order_id);
    EXPECT_EQ(val4->status, Execution_status::fully_filled);

    const auto val5 = output_buffer_.pop();
    EXPECT_EQ(val5->order_id, order4.order_id);
    EXPECT_EQ(val5->status, Execution_status::partially_filled);

    const auto val6 = output_buffer_.pop();
    EXPECT_EQ(val6->order_id, order2.order_id);
    EXPECT_EQ(val6->status, Execution_status::fully_filled);

    const auto val7 = output_buffer_.pop();
    EXPECT_EQ(val7->order_id, order4.order_id);
    EXPECT_EQ(val7->status, Execution_status::partially_filled);

    const auto val8 = output_buffer_.pop();
    EXPECT_EQ(val8->order_id, order3.order_id);
    EXPECT_EQ(val8->status, Execution_status::fully_filled);

    const auto val9 = output_buffer_.pop();
    EXPECT_EQ(val9->order_id, order4.order_id);
    EXPECT_EQ(val9->status, Execution_status::fully_filled);
}

TEST_F(MatchingEngineTest, MultipleLevelFill) {
    Order order1 = create_order(Instrument_id::rose, Side::sell, 100, 1000);

    Order order2 = create_order(Instrument_id::rose, Side::sell, 100, 1100);

    Order order3 = create_order(Instrument_id::rose, Side::sell, 100, 1200);

    Order order4 = create_order(Instrument_id::rose, Side::buy, 300, 1200);

    input_buffer_.push(order1);
    input_buffer_.push(order2);
    input_buffer_.push(order3);
    input_buffer_.push(order4);

    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());
    EXPECT_TRUE(matching_engine_.step());

    auto val1 = output_buffer_.pop();
    EXPECT_EQ(val1->order_id, order1.order_id);
    EXPECT_EQ(val1->status, Execution_status::newly_created);

    const auto val2 = output_buffer_.pop();
    EXPECT_EQ(val2->order_id, order2.order_id);
    EXPECT_EQ(val2->status, Execution_status::newly_created);

    const auto val3 = output_buffer_.pop();
    EXPECT_EQ(val3->order_id, order3.order_id);
    EXPECT_EQ(val3->status, Execution_status::newly_created);

    const auto val4 = output_buffer_.pop();
    EXPECT_EQ(val4->order_id, order1.order_id);
    EXPECT_EQ(val4->status, Execution_status::fully_filled);

    const auto val5 = output_buffer_.pop();
    EXPECT_EQ(val5->order_id, order4.order_id);
    EXPECT_EQ(val5->status, Execution_status::partially_filled);
    EXPECT_EQ(val5->price, order1.price);

    const auto val6 = output_buffer_.pop();
    EXPECT_EQ(val6->order_id, order2.order_id);
    EXPECT_EQ(val6->status, Execution_status::fully_filled);

    const auto val7 = output_buffer_.pop();
    EXPECT_EQ(val7->order_id, order4.order_id);
    EXPECT_EQ(val7->status, Execution_status::partially_filled);
    EXPECT_EQ(val7->price, order2.price);

    const auto val8 = output_buffer_.pop();
    EXPECT_EQ(val8->order_id, order3.order_id);
    EXPECT_EQ(val8->status, Execution_status::fully_filled);

    const auto val9 = output_buffer_.pop();
    EXPECT_EQ(val9->order_id, order4.order_id);
    EXPECT_EQ(val9->status, Execution_status::fully_filled);
    EXPECT_EQ(val9->price, order3.price);
}

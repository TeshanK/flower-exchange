#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "matching/order_processor.h"

namespace {

InboundOrderMsg make_message(const char *coid, const char *instrument, int side,
                             int quantity, double price, uint64_t sequence) {
  InboundOrderMsg message{};
  std::strncpy(message.coid, coid, sizeof(message.coid) - 1);
  std::strncpy(message.instrument, instrument, sizeof(message.instrument) - 1);
  message.side = side;
  message.quantity = quantity;
  message.price = price;
  message.seq = sequence;
  return message;
}

void finish_input(InboundOrderQueue &queue, PipelineState &state) {
  InboundOrderMsg end{};
  end.end_of_stream = true;
  ASSERT_TRUE(queue.push(end));
  state.producer_done.store(true);
}

} // namespace

TEST(OrderProcessorTest, ProducesRejectAndValidReportsWithProgressingIds) {
  OrderProcessor processor;
  InboundOrderQueue inbound;
  OutboundReportQueue outbound;
  PipelineState state;

  ASSERT_TRUE(inbound.push(make_message("bad", "", 1, 100, 55.0, 1)));
  ASSERT_TRUE(
      inbound.push(make_message("good", "Rose", 2, 100, 45.0, 2)));
  finish_input(inbound, state);

  processor.consume(inbound, outbound, state);

  EXPECT_TRUE(state.matcher_done.load());
  EXPECT_EQ(state.consumed_orders.load(), 2u);
  OutboundReportMsg rejected{};
  OutboundReportMsg accepted{};
  ASSERT_TRUE(outbound.pop(rejected));
  ASSERT_TRUE(outbound.pop(accepted));
  EXPECT_STREQ(rejected.oid, "ord1");
  EXPECT_STREQ(rejected.exec_status, "Reject");
  EXPECT_STREQ(rejected.reason, "Invalid instrument");
  EXPECT_STREQ(accepted.oid, "ord2");
  EXPECT_STREQ(accepted.exec_status, "New");
  EXPECT_TRUE(outbound.empty());
}

TEST(OrderProcessorTest, ResetClearsBooksAndRestartsOrderIds) {
  OrderProcessor processor;

  auto inbound = std::make_unique<InboundOrderQueue>();
  auto outbound = std::make_unique<OutboundReportQueue>();
  PipelineState first_state;
  ASSERT_TRUE(inbound->push(
      make_message("sell", "Rose", 2, 100, 45.0, 1)));
  finish_input(*inbound, first_state);
  processor.consume(*inbound, *outbound, first_state);

  OutboundReportMsg first_report{};
  ASSERT_TRUE(outbound->pop(first_report));
  EXPECT_STREQ(first_report.oid, "ord1");
  EXPECT_STREQ(first_report.exec_status, "New");

  processor.reset();

  PipelineState second_state;
  ASSERT_TRUE(inbound->push(
      make_message("buy", "Rose", 1, 100, 45.0, 1)));
  finish_input(*inbound, second_state);
  processor.consume(*inbound, *outbound, second_state);

  OutboundReportMsg second_report{};
  ASSERT_TRUE(outbound->pop(second_report));
  EXPECT_STREQ(second_report.oid, "ord1");
  EXPECT_STREQ(second_report.exec_status, "New");
  EXPECT_TRUE(outbound->empty());
}

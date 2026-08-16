#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

#include "io/csv_order_producer.h"

namespace {

std::filesystem::path unique_input_path(const std::string &stem) {
  return std::filesystem::temp_directory_path() /
         (stem + std::to_string(std::chrono::steady_clock::now()
                                    .time_since_epoch()
                                    .count()) +
          ".csv");
}

} // namespace

TEST(CsvOrderProducerTest, PublishesParsedRowsAndEndOfStream) {
  const auto input_path = unique_input_path("flower_exchange_producer_");
  {
    std::ofstream output(input_path);
    output << "Client Order ID,Instrument,Side,Quantity,Price\n";
    output << "c1,Rose,1,100,55.25\n";
    output << "short,row\n";
    output << "c2,Tulip,2,200,12.50\n";
  }

  auto queue = std::make_unique<InboundOrderQueue>();
  PipelineState state;
  std::ostringstream diagnostics;
  CsvOrderProducer producer(diagnostics);
  producer.produce(input_path.string(), *queue, state);

  EXPECT_TRUE(state.producer_done.load());
  EXPECT_EQ(state.produced_orders.load(), 2u);
  EXPECT_TRUE(diagnostics.str().empty());

  InboundOrderMsg first{};
  InboundOrderMsg second{};
  InboundOrderMsg end{};
  ASSERT_TRUE(queue->pop(first));
  ASSERT_TRUE(queue->pop(second));
  ASSERT_TRUE(queue->pop(end));
  EXPECT_STREQ(first.coid, "c1");
  EXPECT_STREQ(first.instrument, "Rose");
  EXPECT_EQ(first.side, 1);
  EXPECT_EQ(first.quantity, 100);
  EXPECT_DOUBLE_EQ(first.price, 55.25);
  EXPECT_EQ(first.seq, 1u);
  EXPECT_STREQ(second.coid, "c2");
  EXPECT_EQ(second.seq, 2u);
  EXPECT_TRUE(end.end_of_stream);
  EXPECT_TRUE(queue->empty());

  std::filesystem::remove(input_path);
}

TEST(CsvOrderProducerTest, MissingInputPublishesOnlyEndOfStream) {
  const auto input_path = unique_input_path("flower_exchange_missing_");
  auto queue = std::make_unique<InboundOrderQueue>();
  PipelineState state;
  std::ostringstream diagnostics;
  CsvOrderProducer producer(diagnostics);
  producer.produce(input_path.string(), *queue, state);

  EXPECT_TRUE(state.producer_done.load());
  EXPECT_EQ(state.produced_orders.load(), 0u);
  EXPECT_NE(diagnostics.str().find("Unable to open input file"),
            std::string::npos);

  InboundOrderMsg end{};
  ASSERT_TRUE(queue->pop(end));
  EXPECT_TRUE(end.end_of_stream);
  EXPECT_TRUE(queue->empty());
}

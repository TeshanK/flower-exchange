#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include "io/csv_report_writer.h"

namespace {

std::filesystem::path unique_temp_directory() {
  return std::filesystem::temp_directory_path() /
         ("flower_exchange_writer_" +
          std::to_string(std::chrono::steady_clock::now()
                             .time_since_epoch()
                             .count()));
}

template <std::size_t N>
uint8_t copy_field(char (&destination)[N], const char *source) {
  const std::size_t length = std::strlen(source);
  std::memcpy(destination, source, length);
  destination[length] = '\0';
  return static_cast<uint8_t>(length);
}

} // namespace

TEST(CsvReportWriterTest, DerivesPathAndWritesExactCsv) {
  const auto output_directory = unique_temp_directory();
  std::ostringstream diagnostics;
  CsvReportWriter writer(output_directory, diagnostics);

  ASSERT_TRUE(writer.prepare("somewhere/orders.csv"));
  EXPECT_EQ(writer.output_path(),
            (output_directory / "orders_reports.csv").string());

  OutboundReportQueue queue;
  OutboundReportMsg message{};
  message.oid_len = copy_field(message.oid, "ord1");
  message.coid_len = copy_field(message.coid, "client1");
  message.instrument_len = copy_field(message.instrument, "Rose");
  message.side = 1;
  message.exec_status_len = copy_field(message.exec_status, "New");
  message.quantity = 100;
  message.price_text_len = copy_field(message.price_text, "55.00");
  message.reason_len = copy_field(message.reason, "");
  message.timestamp_len = copy_field(message.timestamp, "20260816-120000.000");
  ASSERT_TRUE(queue.push(message));

  std::atomic<bool> matcher_done{true};
  const uint64_t elapsed_ns = writer.drain(queue, matcher_done);
  EXPECT_GE(elapsed_ns, 0u);

  std::ifstream input(writer.output_path(), std::ios::binary);
  std::ostringstream contents;
  contents << input.rdbuf();
  EXPECT_EQ(contents.str(),
            "Order ID,Client Order Id,Instrument,Side,Exec Status,Quantity,"
            "Price,Reason,Timestamp\n"
            "ord1,client1,Rose,1,New,100,55.00,,20260816-120000.000\n");
  EXPECT_TRUE(diagnostics.str().empty());

  std::filesystem::remove_all(output_directory);
}

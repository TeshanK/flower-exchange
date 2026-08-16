#include <gtest/gtest.h>

#include <sstream>
#include <string>

#include "app/processing_reporter.h"

TEST(ConsoleProcessingReporterTest, PreservesConsoleOutputShape) {
  std::ostringstream output;
  ConsoleProcessingReporter reporter(output);
  const ProcessingStats stats{
      .produced_orders = 42,
      .consumed_orders = 41,
      .total_sec = 2.0,
      .producer_sec = 0.5,
      .matcher_sec = 1.0,
      .writer_sec = 1.5,
      .orders_per_sec = 21.0,
  };

  reporter.report("output/orders_reports.csv", stats);

  EXPECT_EQ(output.str(),
            "Wrote reports to output/orders_reports.csv\n"
            "PERF orders=42 consumed=41 total_sec=2 orders_per_sec=21\n"
            " producer_sec=0.5 matcher_sec=1 write_sec=1.5\n");
}

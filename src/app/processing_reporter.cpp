#include "app/processing_reporter.h"

#include <print>

ConsoleProcessingReporter::ConsoleProcessingReporter(std::ostream &output)
    : output_(output) {}

void ConsoleProcessingReporter::report(const std::string &output_path,
                                       const ProcessingStats &stats) {
  std::println(output_, "Wrote reports to {}", output_path);
  std::println(output_,
               "PERF orders={} consumed={} total_sec={} orders_per_sec={}",
               stats.produced_orders, stats.consumed_orders, stats.total_sec,
               stats.orders_per_sec);
  std::println(output_, " producer_sec={} matcher_sec={} write_sec={}",
               stats.producer_sec, stats.matcher_sec, stats.writer_sec);
}

#include "app/application.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <print>
#include <sstream>
#include <string>

#include "common/thread_utils.h"
#include "io/csv_order_producer.h"
#include "io/csv_report_writer.h"

Application::Application() = default;

void Application::process_file(const std::string &input_path) {
  const auto total_start = std::chrono::steady_clock::now();

  while (!inbound_queue_.empty()) {
    InboundOrderMsg message{};
    inbound_queue_.pop(message);
  }
  while (!outbound_queue_.empty()) {
    OutboundReportMsg message{};
    outbound_queue_.pop(message);
  }
  order_processor_.reset();

  CsvReportWriter report_writer;
  if (!report_writer.prepare(input_path)) {
    return;
  }
  const std::string output_path = report_writer.output_path();

  PipelineState state;
  CsvOrderProducer order_producer;

  auto producer = createAndStartThread(
      0, "io-producer", &CsvOrderProducer::produce, &order_producer, input_path,
      std::ref(inbound_queue_), std::ref(state));

  auto matcher = createAndStartThread(
      1, "matcher", &OrderProcessor::consume, &order_processor_,
      std::ref(inbound_queue_), std::ref(outbound_queue_), std::ref(state));

  const uint64_t writer_ns =
      report_writer.drain(outbound_queue_, state.matcher_done);

  if (producer->joinable()) {
    producer->join();
    state.producer_done.store(true);
  }

  if (matcher->joinable()) {
    matcher->join();
    state.matcher_done.store(true);
  }

  const auto write_end = std::chrono::steady_clock::now();
  const double total_sec =
      std::chrono::duration_cast<std::chrono::duration<double>>(write_end -
                                                                total_start)
          .count();
  const double producer_sec =
      static_cast<double>(state.producer_ns.load()) / 1'000'000'000.0;
  const double matcher_sec =
      static_cast<double>(state.matcher_ns.load()) / 1'000'000'000.0;
  const double write_sec = static_cast<double>(writer_ns) / 1'000'000'000.0;
  const uint64_t orders = state.produced_orders.load();
  const uint64_t matched = state.consumed_orders.load();
  const double orders_per_sec =
      (total_sec > 0.0) ? (static_cast<double>(orders) / total_sec) : 0.0;

  std::println("Wrote reports to {}", output_path);
  std::println("PERF orders={} consumed={} total_sec={} orders_per_sec={}",
               orders, matched, total_sec, orders_per_sec);
  std::println(" producer_sec={} matcher_sec={} write_sec={}", producer_sec,
               matcher_sec, write_sec);
}

void Application::run() {
  std::string user_input;
  while (true) {
    std::getline(std::cin, user_input);
    if (user_input == "QUIT") {
      break;
    }

    std::stringstream input(user_input);
    std::string command;
    std::string path;
    input >> command >> path;

    if (command == "PROCESS" && !path.empty()) {
      process_file(path);
    }
  }
}

#include "app/file_processing_pipeline.h"

#include <chrono>
#include <functional>
#include <utility>

#include "common/thread_utils.h"

FileProcessingPipeline::FileProcessingPipeline(
    std::filesystem::path output_directory, std::ostream &diagnostics,
    std::ostream &report_output)
    : order_producer_(diagnostics),
      report_writer_(std::move(output_directory), diagnostics),
      reporter_(report_output) {}

void FileProcessingPipeline::clear_queues() {
  while (!inbound_queue_.empty()) {
    InboundOrderMsg message{};
    inbound_queue_.pop(message);
  }
  while (!outbound_queue_.empty()) {
    OutboundReportMsg message{};
    outbound_queue_.pop(message);
  }
}

void FileProcessingPipeline::process_file(const std::string &input_path) {
  const auto total_start = std::chrono::steady_clock::now();

  clear_queues();
  order_processor_.reset();

  if (!report_writer_.prepare(input_path)) {
    return;
  }
  const std::string output_path = report_writer_.output_path();

  PipelineState state;
  auto producer = createAndStartThread(
      0, "io-producer", &CsvOrderProducer::produce, &order_producer_, input_path,
      std::ref(inbound_queue_), std::ref(state));

  auto matcher = createAndStartThread(
      1, "matcher", &OrderProcessor::consume, &order_processor_,
      std::ref(inbound_queue_), std::ref(outbound_queue_), std::ref(state));

  const uint64_t writer_ns =
      report_writer_.drain(outbound_queue_, state.matcher_done);

  if (producer->joinable()) {
    producer->join();
    state.producer_done.store(true);
  }
  if (matcher->joinable()) {
    matcher->join();
    state.matcher_done.store(true);
  }

  const auto total_end = std::chrono::steady_clock::now();
  const double total_sec =
      std::chrono::duration_cast<std::chrono::duration<double>>(total_end -
                                                                total_start)
          .count();
  const uint64_t produced_orders = state.produced_orders.load();
  const ProcessingStats stats{
      .produced_orders = produced_orders,
      .consumed_orders = state.consumed_orders.load(),
      .total_sec = total_sec,
      .producer_sec =
          static_cast<double>(state.producer_ns.load()) / 1'000'000'000.0,
      .matcher_sec =
          static_cast<double>(state.matcher_ns.load()) / 1'000'000'000.0,
      .writer_sec = static_cast<double>(writer_ns) / 1'000'000'000.0,
      .orders_per_sec =
          total_sec > 0.0 ? static_cast<double>(produced_orders) / total_sec
                          : 0.0,
  };
  reporter_.report(output_path, stats);
}

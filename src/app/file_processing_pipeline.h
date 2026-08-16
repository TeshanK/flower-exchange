#pragma once

#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>

#include "app/file_processor.h"
#include "app/pipeline_types.h"
#include "app/processing_reporter.h"
#include "io/csv_order_producer.h"
#include "io/csv_report_writer.h"
#include "matching/order_processor.h"

class FileProcessingPipeline final : public FileProcessor {
public:
  explicit FileProcessingPipeline(
      std::filesystem::path output_directory = "output",
      std::ostream &diagnostics = std::cerr,
      std::ostream &report_output = std::cout);

  void process_file(const std::string &input_path) override;

private:
  void clear_queues();

  CsvOrderProducer order_producer_;
  OrderProcessor order_processor_;
  CsvReportWriter report_writer_;
  ConsoleProcessingReporter reporter_;
  InboundOrderQueue inbound_queue_;
  OutboundReportQueue outbound_queue_;
};

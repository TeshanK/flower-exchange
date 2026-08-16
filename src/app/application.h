#pragma once

#include <string>

#include "app/pipeline_types.h"
#include "matching/order_processor.h"

class Application {
public:
  Application();
  ~Application() = default;

  // Runs interactive command loop (PROCESS/QUIT).
  void run();

private:
  // Processes one input CSV into one output report file.
  void process_file(const std::string &input_path);

  OrderProcessor order_processor_;
  InboundOrderQueue inbound_queue_;
  OutboundReportQueue outbound_queue_;
};

#pragma once

#include <iostream>
#include <ostream>
#include <string>

#include "app/pipeline_types.h"

class CsvOrderProducer final {
public:
  explicit CsvOrderProducer(std::ostream &diagnostics = std::cerr);

  // Parses one CSV file and publishes messages followed by end-of-stream.
  void produce(const std::string &input_path, InboundOrderQueue &queue,
               PipelineState &state);

private:
  std::ostream &diagnostics_;
};

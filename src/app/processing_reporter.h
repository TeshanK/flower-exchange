#pragma once

#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

struct ProcessingStats final {
  uint64_t produced_orders;
  uint64_t consumed_orders;
  double total_sec;
  double producer_sec;
  double matcher_sec;
  double writer_sec;
  double orders_per_sec;
};

class ProcessingReporter {
public:
  virtual ~ProcessingReporter() = default;

  virtual void report(const std::string &output_path,
                      const ProcessingStats &stats) = 0;
};

class ConsoleProcessingReporter final : public ProcessingReporter {
public:
  explicit ConsoleProcessingReporter(std::ostream &output = std::cout);

  void report(const std::string &output_path,
              const ProcessingStats &stats) override;

private:
  std::ostream &output_;
};

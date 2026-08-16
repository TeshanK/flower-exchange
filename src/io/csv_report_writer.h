#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ostream>
#include <string>

#include "app/pipeline_types.h"

class CsvReportWriter final {
public:
  explicit CsvReportWriter(
      std::filesystem::path output_directory = "output",
      std::ostream &diagnostics = std::cerr);

  // Creates the report file associated with an input path.
  [[nodiscard]] bool prepare(const std::string &input_path);

  // Drains reports until matching completes and returns elapsed nanoseconds.
  [[nodiscard]] uint64_t drain(OutboundReportQueue &queue,
                               const std::atomic<bool> &matcher_done);

  [[nodiscard]] const std::string &output_path() const noexcept;

private:
  static void append_csv_row(std::string &destination,
                             const OutboundReportMsg &message);

  std::filesystem::path output_directory_;
  std::ostream &diagnostics_;
  std::ofstream output_file_;
  std::array<char, 1 << 20> file_buffer_{};
  std::string output_path_;
};

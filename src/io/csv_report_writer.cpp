#include "io/csv_report_writer.h"

#include <array>
#include <charconv>
#include <chrono>
#include <cstring>
#include <print>
#include <thread>

#include "common/macros.h"
#include "common/types.h"

namespace {

const std::array<std::array<char, 5>, MAX_QUANTITY + 1> kQuantityText =
    []() -> std::array<std::array<char, 5>, MAX_QUANTITY + 1> {
  std::array<std::array<char, 5>, MAX_QUANTITY + 1> out{};
  for (int i = 0; i <= MAX_QUANTITY; ++i) {
    auto result =
        std::to_chars(out[i].data(), out[i].data() + out[i].size() - 1, i);
    if (UNLIKELY(result.ec == std::errc())) {
      *result.ptr = '\0';
    } else {
      out[i][0] = '0';
      out[i][1] = '\0';
    }
  }
  return out;
}();

const std::array<uint8_t, MAX_QUANTITY + 1> kQuantityTextLen =
    []() -> std::array<uint8_t, MAX_QUANTITY + 1> {
  std::array<uint8_t, MAX_QUANTITY + 1> out{};
  for (int i = 0; i <= MAX_QUANTITY; ++i) {
    const char *text = kQuantityText[i].data();
    uint8_t length = 0;
    while (text[length] != '\0') {
      ++length;
    }
    out[i] = length;
  }
  return out;
}();

std::size_t write_side_to_buffer(char *out, int side) {
  if (side == static_cast<int>(Side::BUY)) {
    out[0] = '1';
    return 1;
  }
  if (side == static_cast<int>(Side::SELL)) {
    out[0] = '2';
    return 1;
  }
  auto result = std::to_chars(out, out + 16, side);
  if (UNLIKELY(result.ec == std::errc())) {
    return static_cast<std::size_t>(result.ptr - out);
  }
  out[0] = '0';
  return 1;
}

std::size_t write_quantity_to_buffer(char *out, int quantity) {
  if (quantity >= 0 && quantity <= MAX_QUANTITY) {
    const std::size_t length = kQuantityTextLen[quantity];
    if (length > 0) {
      std::memcpy(out, kQuantityText[quantity].data(), length);
    }
    return length;
  }
  auto result = std::to_chars(out, out + 16, quantity);
  if (UNLIKELY(result.ec == std::errc())) {
    return static_cast<std::size_t>(result.ptr - out);
  }
  out[0] = '0';
  return 1;
}

} // namespace

CsvReportWriter::CsvReportWriter(std::filesystem::path output_directory,
                                 std::ostream &diagnostics)
    : output_directory_(std::move(output_directory)),
      diagnostics_(diagnostics) {}

bool CsvReportWriter::prepare(const std::string &input_path) {
  if (output_file_.is_open()) {
    output_file_.close();
  }
  output_file_.clear();

  std::filesystem::create_directories(output_directory_);
  const std::string stem = std::filesystem::path(input_path).stem().string();
  output_path_ = (output_directory_ / (stem + "_reports.csv")).string();

  output_file_.open(output_path_);
  output_file_.rdbuf()->pubsetbuf(
      file_buffer_.data(),
      static_cast<std::streamsize>(file_buffer_.size()));
  if (!output_file_.is_open()) {
    std::println(diagnostics_, "Unable to open output file: {}", output_path_);
    return false;
  }
  return true;
}

uint64_t CsvReportWriter::drain(OutboundReportQueue &queue,
                                const std::atomic<bool> &matcher_done) {
  const auto write_start = std::chrono::steady_clock::now();
  std::string write_batch;
  write_batch.reserve(4 * 1024 * 1024);
  write_batch.append("Order ID,Client Order Id,Instrument,Side,Exec "
                     "Status,Quantity,Price,Reason,Timestamp\n");

  int write_spin = 0;
  while (!matcher_done.load() || !queue.empty()) {
    OutboundReportMsg message{};
    if (!queue.pop(message)) {
      if (write_spin++ < 200) {
        __builtin_ia32_pause();
      } else {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
        write_spin = 0;
      }
      continue;
    }
    write_spin = 0;

    append_csv_row(write_batch, message);
    if (write_batch.size() >= 4 * 1024 * 1024) {
      output_file_.write(write_batch.data(),
                         static_cast<std::streamsize>(write_batch.size()));
      write_batch.clear();
    }
  }

  if (!write_batch.empty()) {
    output_file_.write(write_batch.data(),
                       static_cast<std::streamsize>(write_batch.size()));
  }
  output_file_.close();

  const auto write_end = std::chrono::steady_clock::now();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(write_end -
                                                           write_start)
          .count());
}

const std::string &CsvReportWriter::output_path() const noexcept {
  return output_path_;
}

void CsvReportWriter::append_csv_row(std::string &destination,
                                     const OutboundReportMsg &message) {
  std::array<char, 320> row{};
  char *out = row.data();

  auto append_span = [&](const char *source, std::size_t length) -> void {
    if (length > 0) {
      std::memcpy(out, source, length);
      out += length;
    }
  };

  append_span(message.oid, message.oid_len);
  *out++ = ',';
  append_span(message.coid, message.coid_len);
  *out++ = ',';
  append_span(message.instrument, message.instrument_len);
  *out++ = ',';
  out += write_side_to_buffer(out, message.side);
  *out++ = ',';
  append_span(message.exec_status, message.exec_status_len);
  *out++ = ',';
  out += write_quantity_to_buffer(out, message.quantity);
  *out++ = ',';
  append_span(message.price_text, message.price_text_len);
  *out++ = ',';
  append_span(message.reason, message.reason_len);
  *out++ = ',';
  append_span(message.timestamp, message.timestamp_len);
  *out++ = '\n';

  destination.append(row.data(), static_cast<std::size_t>(out - row.data()));
}

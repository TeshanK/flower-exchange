#include "io/csv_order_producer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <print>
#include <string_view>

#include "common/spsc_utils.h"
#include "common/types.h"
#include "io/csv_reader.h"

namespace {

void copy_text(char *destination, std::size_t destination_size,
               std::string_view source) {
  if (!destination || destination_size == 0) {
    return;
  }
  const std::size_t copy_length =
      std::min(source.size(), destination_size - 1);
  if (copy_length > 0) {
    std::memcpy(destination, source.data(), copy_length);
  }
  destination[copy_length] = '\0';
}

void publish_end_of_stream(InboundOrderQueue &queue) {
  InboundOrderMsg end{};
  end.end_of_stream = true;
  spin_until_success([&]() -> bool { return queue.push(end); }, 100);
}

} // namespace

CsvOrderProducer::CsvOrderProducer(std::ostream &diagnostics)
    : diagnostics_(diagnostics) {}

void CsvOrderProducer::produce(const std::string &input_path,
                               InboundOrderQueue &queue,
                               PipelineState &state) {
  const auto producer_start = std::chrono::steady_clock::now();
  uint64_t produced = 0;

  std::ifstream file(input_path);
  std::array<char, 262144> file_buffer{};
  file.rdbuf()->pubsetbuf(file_buffer.data(),
                          static_cast<std::streamsize>(file_buffer.size()));

  if (!file.is_open()) {
    std::println(diagnostics_, "Unable to open input file: {}", input_path);
    publish_end_of_stream(queue);
    state.producer_done.store(true);
    return;
  }

  bool first_row = true;
  uint64_t sequence = 1;
  for (const CSVRow &row : CSVRange(file)) {
    if (first_row) {
      first_row = false;
      continue;
    }
    if (row.size() < 5) {
      continue;
    }

    InboundOrderMsg message{};
    message.end_of_stream = false;
    copy_text(message.coid, sizeof(message.coid), row[0]);
    copy_text(message.instrument, sizeof(message.instrument), row[1]);
    message.side = string_view_to_int(row[2]);
    message.quantity = string_view_to_int(row[3]);
    message.price = string_view_to_double(row[4]);
    message.seq = sequence++;
    ++produced;

    spin_until_success([&]() -> bool { return queue.push(message); }, 100);
  }

  publish_end_of_stream(queue);

  const auto producer_end = std::chrono::steady_clock::now();
  state.producer_ns.store(static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(producer_end -
                                                           producer_start)
          .count()));
  state.produced_orders.store(produced);
  state.producer_done.store(true);
}

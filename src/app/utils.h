#include <cstdint>
#include <string_view>
#include <cstring>

#include "common/macros.h"
#include "common/types.h"
#include "app/pipeline_types.h"

std::size_t copy_text_len(char *dst, std::size_t dst_size, const char *src) {
  if (UNLIKELY(!dst || dst_size == 0)) {
    return 0;
  }
  dst[0] = '\0';

  if (UNLIKELY(!src)) {
    return 0;
  }

  const std::size_t max_copy = dst_size - 1;
  std::size_t copy_len = 0;
  while (copy_len < max_copy && src[copy_len] != '\0') {
    dst[copy_len] = src[copy_len];
    ++copy_len;
  }
  dst[copy_len] = '\0';
  return copy_len;
}

void copy_text(char *dst, std::size_t dst_size, std::string_view src) {
  if (UNLIKELY(!dst || dst_size == 0)) {
    return;
  }
  std::size_t copy_len = std::min(src.size(), dst_size - 1);
  if (copy_len > 0) {
    std::memcpy(dst, src.data(), copy_len);
  }
  dst[copy_len] = '\0';
}

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
    const char *p = kQuantityText[i].data();
    uint8_t len = 0;
    while (p[len] != '\0') {
      ++len;
    }
    out[i] = len;
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
    const std::size_t len = kQuantityTextLen[quantity];
    if (len > 0) {
      std::memcpy(out, kQuantityText[quantity].data(), len);
    }
    return len;
  }
  auto result = std::to_chars(out, out + 16, quantity);
  if (UNLIKELY(result.ec == std::errc())) {
    return static_cast<std::size_t>(result.ptr - out);
  }
  out[0] = '0';
  return 1;
}

void fill_outbound_common_fields(OutboundReportMsg &out,
                                 const ExecutionReport *report,
                                 const char *instrument_text, int side) {
  out.oid_len = static_cast<uint8_t>(
      copy_text_len(out.oid, sizeof(out.oid), report->oid));
  out.coid_len = static_cast<uint8_t>(
      copy_text_len(out.coid, sizeof(out.coid), report->coid));
  out.instrument_len = static_cast<uint8_t>(
      copy_text_len(out.instrument, sizeof(out.instrument), instrument_text));
  out.side = side;
  out.exec_status_len = static_cast<uint8_t>(
      copy_text_len(out.exec_status, sizeof(out.exec_status),
                    exec_status_to_string(report->status)));
  out.quantity = report->quantity;
  out.reason_len = static_cast<uint8_t>(
      copy_text_len(out.reason, sizeof(out.reason), report->reason));
  out.timestamp_len = static_cast<uint8_t>(
      copy_text_len(out.timestamp, sizeof(out.timestamp), report->timestamp));
}

void append_csv_row(std::string &dst, const OutboundReportMsg &msg) {
  std::array<char, 320> row{};
  char *out = row.data();

  auto append_span = [&](const char *src, std::size_t len) -> void {
    if (len > 0) {
      std::memcpy(out, src, len);
      out += len;
    }
  };

  append_span(msg.oid, msg.oid_len);
  *out++ = ',';
  append_span(msg.coid, msg.coid_len);
  *out++ = ',';
  append_span(msg.instrument, msg.instrument_len);
  *out++ = ',';
  out += write_side_to_buffer(out, msg.side);
  *out++ = ',';
  append_span(msg.exec_status, msg.exec_status_len);
  *out++ = ',';
  out += write_quantity_to_buffer(out, msg.quantity);
  *out++ = ',';
  append_span(msg.price_text, msg.price_text_len);
  *out++ = ',';
  append_span(msg.reason, msg.reason_len);
  *out++ = ',';
  append_span(msg.timestamp, msg.timestamp_len);
  *out++ = '\n';

  dst.append(row.data(), static_cast<std::size_t>(out - row.data()));
}

void format_order_id(char *oid, std::size_t oid_size, uint64_t id) {
  if (UNLIKELY(!oid || oid_size < 5)) {
    return;
  }
  oid[0] = 'o';
  oid[1] = 'r';
  oid[2] = 'd';
  auto result = std::to_chars(oid + 3, oid + oid_size - 1, id);
  if (UNLIKELY(result.ec == std::errc())) {
    *result.ptr = '\0';
    return;
  }
  oid[0] = '\0';
}

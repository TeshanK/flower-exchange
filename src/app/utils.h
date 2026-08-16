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

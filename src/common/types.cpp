#include "common/types.h"

#include <charconv>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <stdexcept>

#include "common/macros.h"

const std::array<const char *, 5> kValidInstrumentNames = {
    "Rose", "Lavender", "Lotus", "Tulip", "Orchid"};

PriceTick double_to_ticks(double price) {
  if (UNLIKELY(!std::isfinite(price))) {
    return std::numeric_limits<PriceTick>::max();
  }
  if (UNLIKELY(price <= 0.0)) {
    return 0;
  }

  const double scaled = price / TICK_SIZE;
  const auto max_tick =
      static_cast<double>(std::numeric_limits<PriceTick>::max());
  if (UNLIKELY(scaled >= max_tick)) {
    return std::numeric_limits<PriceTick>::max();
  }
  return static_cast<PriceTick>(std::llround(scaled));
}

double ticks_to_double(PriceTick ticks) {
  return static_cast<double>(ticks) * TICK_SIZE;
}

const char *instrument_to_string(InstrumentType instrument) {
  switch (instrument) {
  case InstrumentType::ROSE:
    return "Rose";
  case InstrumentType::LAVENDER:
    return "Lavender";
  case InstrumentType::LOTUS:
    return "Lotus";
  case InstrumentType::TULIP:
    return "Tulip";
  case InstrumentType::ORCHID:
    return "Orchid";
  default:
    return "UNKNOWN";
  }
}

const char *exec_status_to_string(ExecStatus status) {
  switch (status) {
  case ExecStatus::NEW:
    return "New";
  case ExecStatus::REJECTED:
    return "Reject";
  case ExecStatus::FILL:
    return "Fill";
  case ExecStatus::PFILL:
    return "Pfill";
  default:
    return "UNKNOWN";
  }
}

bool parse_instrument(std::string_view instrument, InstrumentType &out) {
  if (instrument == "Rose") {
    out = InstrumentType::ROSE;
    return true;
  }
  if (instrument == "Lavender") {
    out = InstrumentType::LAVENDER;
    return true;
  }
  if (instrument == "Lotus") {
    out = InstrumentType::LOTUS;
    return true;
  }
  if (instrument == "Tulip") {
    out = InstrumentType::TULIP;
    return true;
  }
  if (instrument == "Orchid") {
    out = InstrumentType::ORCHID;
    return true;
  }
  return false;
}

int string_view_to_int(std::string_view sv) {
  if (sv.empty()) {
    throw std::invalid_argument("stoi");
  }

  int result = 0;
  bool negative = false;
  std::size_t i = 0;

  // Skip leading whitespace
  while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t'))
    ++i;
  if (i >= sv.size())
    throw std::invalid_argument("stoi");

  // Check for sign
  if (sv[i] == '-') {
    negative = true;
    ++i;
  } else if (sv[i] == '+') {
    ++i;
  }

  if (i >= sv.size() || sv[i] < '0' || sv[i] > '9') {
    throw std::invalid_argument("stoi");
  }

  // Parse digits
  while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
    const int digit = sv[i] - '0';
    if (result > (std::numeric_limits<int>::max() - digit) / 10) {
      result = std::numeric_limits<int>::max();
      while (i + 1 < sv.size() && sv[i + 1] >= '0' && sv[i + 1] <= '9') {
        ++i;
      }
      ++i;
      break;
    }
    result = result * 10 + digit;
    ++i;
  }

  // Skip trailing whitespace
  while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t'))
    ++i;

  // Must have consumed entire string
  if (i != sv.size()) {
    throw std::invalid_argument("stoi");
  }

  return negative ? -result : result;
}

double string_view_to_double(std::string_view sv) {
  if (sv.empty()) {
    throw std::invalid_argument("stod");
  }

  std::size_t i = 0;

  // Skip leading whitespace
  while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t'))
    ++i;
  if (i >= sv.size())
    throw std::invalid_argument("stod");

  bool negative = false;
  if (sv[i] == '+' || sv[i] == '-') {
    negative = (sv[i] == '-');
    ++i;
  }

  bool has_digit = false;
  double value = 0.0;

  while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
    has_digit = true;
    value = (value * 10.0) + static_cast<double>(sv[i] - '0');
    ++i;
  }

  if (i < sv.size() && sv[i] == '.') {
    ++i;
    double place = 0.1;
    while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
      has_digit = true;
      value += static_cast<double>(sv[i] - '0') * place;
      place *= 0.1;
      ++i;
    }
  }

  if (!has_digit) {
    throw std::invalid_argument("stod");
  }

  if (i < sv.size() && (sv[i] == 'e' || sv[i] == 'E')) {
    ++i;
    bool exp_negative = false;
    if (i < sv.size() && (sv[i] == '+' || sv[i] == '-')) {
      exp_negative = (sv[i] == '-');
      ++i;
    }

    if (i >= sv.size() || sv[i] < '0' || sv[i] > '9') {
      throw std::invalid_argument("stod");
    }

    int exponent = 0;
    while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
      const int digit = sv[i] - '0';
      if (exponent <= (std::numeric_limits<int>::max() - digit) / 10) {
        exponent = exponent * 10 + digit;
      } else {
        exponent = std::numeric_limits<int>::max();
      }
      ++i;
    }

    const int signed_exponent = exp_negative ? -exponent : exponent;
    value *= std::pow(10.0, static_cast<double>(signed_exponent));
  }

  while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t')) {
    ++i;
  }
  if (i != sv.size()) {
    throw std::invalid_argument("stod");
  }

  return negative ? -value : value;
}

/*
 * Formats a price in ticks into a two-decimal string buffer.
 *
 * @ticks: price in ticks
 * @buffer: output buffer to write the formatted price string
 * @buffer_size: size of the output buffer
 *
 * Returns the number of characters written excluding the null terminator, or 0
 * on error.
 */
std::size_t format_ticks_to_buffer(PriceTick ticks, char *buffer,
                                   std::size_t buffer_size) {
  // The minimum price is "0.00" so we need at least 5 bytes to represent a
  // valid price + null terminator
  if (UNLIKELY(!buffer || buffer_size < 5)) {
    return 0;
  }

  const auto integer_part = static_cast<unsigned long long>(ticks / 100ULL);
  const auto frac_part = static_cast<unsigned>(ticks % 100ULL);

  // convert the integer part to string
  auto int_result = std::to_chars(buffer, buffer + buffer_size, integer_part);
  if (UNLIKELY(int_result.ec != std::errc())) {
    buffer[0] = '\0';
    return 0;
  }

  // check if we have enough space for the fraction part ".00" + null
  // terminator so we need 4 bytes
  char *out = int_result.ptr;
  const char *end = buffer + buffer_size;
  if (UNLIKELY(out + 4 > end)) {
    buffer[0] = '\0';
    return 0;
  }

  // append the fractional part
  *out++ = '.';
  *out++ = static_cast<char>('0' + (frac_part / 10U));
  *out++ = static_cast<char>('0' + (frac_part % 10U));
  *out = '\0';
  return static_cast<std::size_t>(out - buffer);
}

/*
 * Formats a double price into a two-decimal string buffer.
 *
 * @price: price as a double
 * @buffer: output buffer to write the formatted price string
 * @buffer_size: size of the output buffer
 *
 * Returns the number of characters written exclusing the null terminator, or 0
 * on error.
 */
std::size_t format_price_to_buffer(double price, char *buffer,
                                   std::size_t buffer_size) {
  if (UNLIKELY(!buffer || buffer_size == 0)) {
    return 0;
  }

  if (price >= 0.0) {
    const std::size_t len =
        format_ticks_to_buffer(double_to_ticks(price), buffer, buffer_size);
    if (len > 0) {
      return len;
    }
  }

  if (UNLIKELY(!std::isfinite(price) || buffer_size < 5)) {
    buffer[0] = '\0';
    return 0;
  }

  const double scaled = price * 100.0;
  if (UNLIKELY(
          scaled > static_cast<double>(std::numeric_limits<long long>::max()) ||
          scaled <
              static_cast<double>(std::numeric_limits<long long>::min()))) {
    buffer[0] = '\0';
    return 0;
  }

  long long cents = std::llround(scaled);
  const bool negative = cents < 0;
  unsigned long long abs_cents =
      negative ? static_cast<unsigned long long>(-(cents + 1)) + 1ULL
               : static_cast<unsigned long long>(cents);

  const unsigned long long integer_part = abs_cents / 100ULL;
  const auto frac_part = static_cast<unsigned>(abs_cents % 100ULL);

  char *out = buffer;
  char *end = buffer + buffer_size;

  if (UNLIKELY(negative)) {
    if (out + 1 >= end) {
      buffer[0] = '\0';
      return 0;
    }
    *out++ = '-';
  }

  auto int_result = std::to_chars(out, end, integer_part);
  if (UNLIKELY(int_result.ec != std::errc())) {
    buffer[0] = '\0';
    return 0;
  }
  out = int_result.ptr;

  if (UNLIKELY(out + 4 > end)) {
    buffer[0] = '\0';
    return 0;
  }

  *out++ = '.';
  *out++ = static_cast<char>('0' + (frac_part / 10U));
  *out++ = static_cast<char>('0' + (frac_part % 10U));
  *out = '\0';
  return static_cast<std::size_t>(out - buffer);
}

#include "common/timestamp_cache.h"

#include <algorithm>

#include "common/macros.h"
#include "common/types.h"

TimestampCache::TimestampCache() = default;

TimestampCache::~TimestampCache() { stop(); }

void TimestampCache::start(std::chrono::milliseconds refresh_interval) {
  if (running_.exchange(true, std::memory_order_release)) {
    return;
  }

  refresh_once();
  worker_ = std::thread([this, refresh_interval]() {
    while (running_.load(std::memory_order_acquire)) {
      refresh_once();
      std::this_thread::sleep_for(refresh_interval);
    }
  });
}

void TimestampCache::stop() {
  if (!running_.exchange(false, std::memory_order_release)) {
    return;
  }
  if (worker_.joinable()) {
    worker_.join();
  }
}

void TimestampCache::snapshot(char *out, std::size_t out_size) const {
  if (UNLIKELY(!out || out_size == 0)) {
    return;
  }

  std::lock_guard<std::mutex> lock(mutex_);
  const std::size_t copy_len = std::min(current_timestamp_.size(), out_size);
  std::copy_n(current_timestamp_.data(), copy_len, out);
  if (copy_len == out_size) {
    out[out_size - 1] = '\0';
  }
}

void TimestampCache::refresh_once() {
  std::array<char, 20> next{};
  ExecutionReport::fill_current_timestamp(next.data(), next.size());

  std::lock_guard<std::mutex> lock(mutex_);
  current_timestamp_ = next;
}

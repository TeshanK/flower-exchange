#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>

class TimestampCache final {
public:
  TimestampCache();
  ~TimestampCache();

  void start(std::chrono::milliseconds refresh_interval =
                 std::chrono::milliseconds(1));
  void stop();
  void snapshot(char *out, std::size_t out_size) const;

private:
  void refresh_once();

  mutable std::mutex mutex_;
  std::array<char, 20> current_timestamp_{};
  std::atomic<bool> running_{false};
  std::thread worker_;
};

#pragma once

#include <chrono>
#include <thread>
#include <utility>

template <typename TryOp>
void spin_until_success(TryOp &&try_op, int fast_spins) {
  int spin_count = 0;
  while (!std::forward<TryOp>(try_op)()) {
    if (spin_count++ < fast_spins) {
      __builtin_ia32_pause();
    } else {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
      spin_count = 0;
    }
  }
}

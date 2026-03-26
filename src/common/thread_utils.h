#pragma once

#include <pthread.h>

#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

/*
 * Pins current thread to a specific core.
 * @core_id: core index to pin the thread to
 * Returns true on success, false on failure.
 */
inline bool setThreadCore(int core_id) noexcept {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);
  return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset) ==
         0;
}

/*
 * Creates and starts a thread with given function and arguments, pins it to a
 * specific core
 *
 * @core_id: core index to pin the thread to, set to -1 for inherited affinity
 * @name: name of the thread
 * @func: function to run in the thread
 * @args: arguments to pass to the function
 *
 * Returns unique_ptr to the created thread object.
 */
template <typename T, typename... Args>
inline std::unique_ptr<std::thread>
createAndStartThread(int core_id, const std::string &name, T &&func,
                     Args &&...args) noexcept {
  using FuncType = std::decay_t<T>;
  auto t = std::make_unique<std::thread>(
      [core_id, name, fn = FuncType(std::forward<T>(func)),
       packed_args = std::make_tuple(std::forward<Args>(args)...)]() {
        if (core_id >= 0 && !setThreadCore(core_id)) {
          std::cerr << "Failed to set thread affinity for " << name << " to "
                    << core_id << '\n';
          std::exit(EXIT_FAILURE);
        }

        std::apply(
            [&](auto &&...unpacked) {
              std::invoke(fn, std::forward<decltype(unpacked)>(unpacked)...);
            },
            packed_args);
      });
  return t;
}

//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#pragma once

#include <vector>
#include <functional>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>

namespace KokkosTools {
namespace PowerProfiler {

template <typename DataType>
class Daemon {
 public:
  using InitCallback     = std::function<void()>;
  using MeasureCallback  = std::function<DataType()>;
  using FinalizeCallback = std::function<void()>;

  Daemon(InitCallback init_cb, MeasureCallback measure_cb,
         FinalizeCallback finalize_cb, std::chrono::microseconds interval,
         size_t initial_capacity = 1000)
      : init_callback_(std::move(init_cb)),
        measure_callback_(std::move(measure_cb)),
        finalize_callback_(std::move(finalize_cb)),
        interval_(interval),
        running_(false),
        stop_requested_(false),
        error_occurred_(false) {
    collected_data_.reserve(initial_capacity);
  }

  ~Daemon();

  void start();
  void stop();

  std::vector<DataType> get_collected_data() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return collected_data_;
  }

  struct TimingStats {
    std::chrono::microseconds min_interval{std::chrono::microseconds::max()};
    std::chrono::microseconds max_interval{std::chrono::microseconds::min()};
    std::chrono::microseconds avg_interval{0};
    size_t measurements_count{0};
    size_t missed_deadlines{0};
  };

  TimingStats get_timing_stats() const {
    std::lock_guard<std::mutex> lock(data_mutex_);
    return timing_stats_;
  }

  bool has_error() const { return error_occurred_.load(); }
  std::string get_last_error() const {
    std::lock_guard<std::mutex> lock(error_mutex_);
    return last_error_;
  }

 private:
  void monitoring_loop();
  void handle_error(const std::exception& e);

  const InitCallback init_callback_;
  const MeasureCallback measure_callback_;
  const FinalizeCallback finalize_callback_;
  const std::chrono::microseconds interval_;

  mutable std::mutex data_mutex_;
  std::vector<DataType> collected_data_;

  // Add timing diagnostics
  mutable TimingStats timing_stats_;
  std::chrono::steady_clock::time_point last_measurement_time_;

  std::thread monitoring_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> stop_requested_;

  std::atomic<bool> error_occurred_;
  mutable std::mutex error_mutex_;
  std::string last_error_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

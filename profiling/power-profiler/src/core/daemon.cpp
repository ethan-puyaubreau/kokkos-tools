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

#include "daemon.hpp"
#include "../data/energy_metrics.hpp"
#include <iostream>

namespace KokkosTools {
namespace PowerProfiler {

template <typename DataType>
Daemon<DataType>::~Daemon() {
  if (running_) {
    stop();
  }
}

template <typename DataType>
void Daemon<DataType>::start() {
  if (running_) {
    return;
  }

  running_        = true;
  stop_requested_ = false;
  error_occurred_ = false;

  try {
    if (init_callback_) {
      init_callback_();
    }

    monitoring_thread_ = std::thread(&Daemon::monitoring_loop, this);
  } catch (const std::exception& e) {
    handle_error(e);
    running_ = false;
    throw;
  }
}

template <typename DataType>
void Daemon<DataType>::stop() {
  if (!running_) {
    return;
  }

  stop_requested_ = true;

  if (monitoring_thread_.joinable()) {
    monitoring_thread_.join();
  }

  try {
    if (finalize_callback_) {
      finalize_callback_();
    }
  } catch (const std::exception& e) {
    handle_error(e);
  }

  running_ = false;
}

template <typename DataType>
void Daemon<DataType>::monitoring_loop() {
  auto next_wake_time    = std::chrono::steady_clock::now();
  last_measurement_time_ = next_wake_time;

  while (!stop_requested_) {
    auto measurement_start = std::chrono::steady_clock::now();
    auto actual_interval =
        std::chrono::duration_cast<std::chrono::microseconds>(
            measurement_start - last_measurement_time_);

    try {
      if (measure_callback_) {
        DataType data = measure_callback_();

        {
          std::lock_guard<std::mutex> lock(data_mutex_);
          collected_data_.push_back(std::move(data));

          if (timing_stats_.measurements_count > 0) {
            timing_stats_.min_interval =
                std::min(timing_stats_.min_interval, actual_interval);
            timing_stats_.max_interval =
                std::max(timing_stats_.max_interval, actual_interval);

            // Running average calculation
            auto total_time =
                timing_stats_.avg_interval * timing_stats_.measurements_count +
                actual_interval;
            timing_stats_.avg_interval =
                total_time / (timing_stats_.measurements_count + 1);
          }
          timing_stats_.measurements_count++;
        }
      }
    } catch (const std::exception& e) {
      handle_error(e);
    }

    last_measurement_time_ = measurement_start;

    next_wake_time = measurement_start + interval_;

    auto now = std::chrono::steady_clock::now();
    if (next_wake_time > now) {
      std::this_thread::sleep_until(next_wake_time);
    } else {
      auto missed_cycles = (now - next_wake_time) / interval_ + 1;
      next_wake_time += missed_cycles * interval_;

      {
        std::lock_guard<std::mutex> lock(data_mutex_);
        timing_stats_.missed_deadlines++;
      }
    }
  }
}

template <typename DataType>
void Daemon<DataType>::handle_error(const std::exception& e) {
  error_occurred_ = true;
  {
    std::lock_guard<std::mutex> lock(error_mutex_);
    last_error_ = e.what();
  }
  std::cerr << "PowerProfiler::Daemon error: " << e.what() << "\n";
}

// Explicit template instantiation
template class Daemon<EnergyReading>;

}  // namespace PowerProfiler
}  // namespace KokkosTools

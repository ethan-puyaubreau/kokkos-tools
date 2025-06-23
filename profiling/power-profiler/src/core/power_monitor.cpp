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

#include "power_monitor.hpp"
#include <iostream>

namespace KokkosTools {
namespace PowerProfiler {

PowerMonitor::PowerMonitor(std::unique_ptr<EnergyProvider> provider,
                           std::chrono::microseconds interval,
                           size_t initial_capacity)
    : energy_provider_(std::move(provider)), started_(false) {
  if (!energy_provider_) {
    return;
  }

  auto init_callback = [this]() {
    try {
      energy_provider_->initialize();
    } catch (const std::exception& e) {
      std::cerr << "PowerProfiler: Energy provider initialization failed: "
                << e.what() << "\n";
      throw;
    }
  };

  auto measure_callback = [this]() {
    return energy_provider_->get_current_reading();
  };

  auto finalize_callback = [this]() {
    try {
      energy_provider_->finalize();
    } catch (const std::exception& e) {
      std::cerr << "PowerProfiler: Energy provider finalization failed: "
                << e.what() << "\n";
    }
  };

  daemon_ = std::make_unique<Daemon<EnergyReading>>(
      init_callback, measure_callback, finalize_callback, interval,
      initial_capacity);
}

bool PowerMonitor::start() {
  if (!daemon_ || started_) {
    return false;
  }

  try {
    daemon_->start();
    started_ = true;
    std::cout << "PowerProfiler: Started background power monitoring.\n";
    return true;
  } catch (const std::exception& e) {
    std::cerr << "PowerProfiler: Failed to start energy monitoring: "
              << e.what() << "\n";
    return false;
  }
}

void PowerMonitor::stop() {
  if (daemon_ && started_) {
    daemon_->stop();
    started_ = false;
  }
}

std::vector<EnergyReading> PowerMonitor::get_collected_data() const {
  if (daemon_) {
    return daemon_->get_collected_data();
  }
  return {};
}

bool PowerMonitor::has_error() const {
  return daemon_ ? daemon_->has_error() : false;
}

std::string PowerMonitor::get_last_error() const {
  return daemon_ ? daemon_->get_last_error() : "";
}

PowerMonitor::TimingStats PowerMonitor::get_timing_stats() const {
  return daemon_ ? daemon_->get_timing_stats() : TimingStats{};
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

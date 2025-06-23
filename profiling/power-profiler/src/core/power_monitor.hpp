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

#include "daemon.hpp"
#include "../providers/energy_provider.hpp"
#include "../data/energy_metrics.hpp"
#include <memory>
#include <chrono>

namespace KokkosTools {
namespace PowerProfiler {

class PowerMonitor {
 public:
  explicit PowerMonitor(
      std::unique_ptr<EnergyProvider> provider,
      std::chrono::microseconds interval = std::chrono::microseconds(20000),
      size_t initial_capacity            = 10000);

  ~PowerMonitor() = default;

  bool start();
  void stop();

  std::vector<EnergyReading> get_collected_data() const;
  bool has_error() const;
  std::string get_last_error() const;

  using TimingStats = Daemon<EnergyReading>::TimingStats;
  TimingStats get_timing_stats() const;

 private:
  std::unique_ptr<EnergyProvider> energy_provider_;
  std::unique_ptr<Daemon<EnergyReading>> daemon_;
  bool started_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

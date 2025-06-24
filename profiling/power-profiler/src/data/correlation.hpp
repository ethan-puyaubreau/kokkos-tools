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

#include "energy_metrics.hpp"
#include "timing_data.hpp"
#include <vector>
#include <chrono>

namespace KokkosTools {
namespace PowerProfiler {

struct KernelEnergyCorrelation {
  KernelTiming kernel;
  std::vector<EnergyReading> energy_readings;
  double total_energy_joules{0.0};
  double average_power_watts{0.0};

  KernelEnergyCorrelation() = default;
  KernelEnergyCorrelation(KernelTiming k) : kernel(std::move(k)) {}

  void calculate_energy_metrics();
};

struct RegionEnergyCorrelation {
  RegionTiming region;
  std::vector<EnergyReading> energy_readings;
  std::vector<KernelEnergyCorrelation> kernels_in_region;
  double total_energy_joules{0.0};
  double average_power_watts{0.0};

  RegionEnergyCorrelation() = default;
  RegionEnergyCorrelation(RegionTiming r) : region(std::move(r)) {}

  void calculate_energy_metrics();
};

class EnergyCorrelator {
 public:
  std::vector<KernelEnergyCorrelation> correlate_kernels_with_energy(
      const std::vector<KernelTiming>& kernels,
      const std::vector<EnergyReading>& energy_readings) const;

  std::vector<RegionEnergyCorrelation> correlate_regions_with_energy(
      const std::vector<RegionTiming>& regions,
      const std::vector<EnergyReading>& energy_readings,
      const std::vector<KernelTiming>& kernels) const;

 private:
  std::vector<EnergyReading> filter_readings_by_time_range(
      const std::vector<EnergyReading>& readings,
      std::chrono::time_point<std::chrono::system_clock> start_time,
      std::chrono::time_point<std::chrono::system_clock> end_time) const;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

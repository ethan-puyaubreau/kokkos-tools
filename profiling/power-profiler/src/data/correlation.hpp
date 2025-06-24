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
#include <map>

namespace KokkosTools {
namespace PowerProfiler {

struct KernelEnergyCorrelation {
  uint64_t kernel_id;         // Reference to the kernel by ID
  std::vector<EnergyReading> energy_readings;
  std::map<uint32_t, double> energy_by_device; // Energy in joules per device
  double total_energy_joules{0.0};
  double average_power_watts{0.0};

  KernelEnergyCorrelation() = default;
  KernelEnergyCorrelation(uint64_t id) : kernel_id(id) {}
  KernelEnergyCorrelation(const KernelTiming& k) : kernel_id(k.id) {}

  void calculate_energy_metrics(const KernelTiming& kernel);
};

struct RegionEnergyCorrelation {
  uint64_t region_id;         // Reference to the region by ID
  std::vector<EnergyReading> energy_readings;
  std::vector<uint64_t> kernel_ids;  // References to kernels by ID
  std::map<uint32_t, double> energy_by_device; // Energy in joules per device
  double total_energy_joules{0.0};
  double average_power_watts{0.0};

  RegionEnergyCorrelation() = default;
  RegionEnergyCorrelation(uint64_t id) : region_id(id) {}
  RegionEnergyCorrelation(const RegionTiming& r) : region_id(r.id) {}

  void calculate_energy_metrics(const RegionTiming& region);
  void add_kernel(uint64_t kernel_id);
};

class EnergyCorrelator {
 public:
  // Calculate energy for a specific time window by interpolating from power readings
  static double calculate_energy_joules(
      const std::vector<EnergyReading>& readings,
      std::chrono::time_point<std::chrono::system_clock> start_time,
      std::chrono::time_point<std::chrono::system_clock> end_time,
      uint32_t device_id = UINT32_MAX);
  
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

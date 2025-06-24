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

#include "correlation.hpp"
#include <cmath>

namespace KokkosTools {
namespace PowerProfiler {

void KernelEnergyCorrelation::calculate_energy_metrics() {
  if (energy_readings.empty()) {
    total_energy_joules = 0.0;
    average_power_watts = 0.0;
    return;
  }

  double total_power_sum = 0.0;
  auto duration_seconds =
      std::chrono::duration<double>(kernel.duration()).count();

  for (const auto& reading : energy_readings) {
    for (const auto& device_power : reading.device_powers) {
      total_power_sum += device_power.second;
    }
  }

  average_power_watts = total_power_sum / energy_readings.size();
  total_energy_joules = average_power_watts * duration_seconds;
}

void RegionEnergyCorrelation::calculate_energy_metrics() {
  if (energy_readings.empty()) {
    total_energy_joules = 0.0;
    average_power_watts = 0.0;
    return;
  }

  double total_power_sum = 0.0;
  auto duration_seconds =
      std::chrono::duration<double>(region.duration()).count();

  for (const auto& reading : energy_readings) {
    for (const auto& device_power : reading.device_powers) {
      total_power_sum += device_power.second;
    }
  }

  average_power_watts = total_power_sum / energy_readings.size();
  total_energy_joules = average_power_watts * duration_seconds;
}

std::vector<KernelEnergyCorrelation>
EnergyCorrelator::correlate_kernels_with_energy(
    const std::vector<KernelTiming>& kernels,
    const std::vector<EnergyReading>& energy_readings) const {
  std::vector<KernelEnergyCorrelation> correlations;
  correlations.reserve(kernels.size());

  for (const auto& kernel : kernels) {
    KernelEnergyCorrelation correlation(kernel);

    correlation.energy_readings = filter_readings_by_time_range(
        energy_readings, kernel.start_time, kernel.end_time);

    correlation.calculate_energy_metrics();
    correlations.push_back(std::move(correlation));
  }

  return correlations;
}

std::vector<RegionEnergyCorrelation>
EnergyCorrelator::correlate_regions_with_energy(
    const std::vector<RegionTiming>& regions,
    const std::vector<EnergyReading>& energy_readings,
    const std::vector<KernelTiming>& kernels) const {
  std::vector<RegionEnergyCorrelation> correlations;
  correlations.reserve(regions.size());

  for (const auto& region : regions) {
    RegionEnergyCorrelation correlation(region);

    correlation.energy_readings = filter_readings_by_time_range(
        energy_readings, region.start_time, region.end_time);

    for (const auto& kernel : kernels) {
      if (kernel.start_time >= region.start_time &&
          kernel.end_time <= region.end_time) {
        KernelEnergyCorrelation kernel_corr(kernel);
        kernel_corr.energy_readings = filter_readings_by_time_range(
            energy_readings, kernel.start_time, kernel.end_time);
        kernel_corr.calculate_energy_metrics();

        correlation.kernels_in_region.push_back(std::move(kernel_corr));
      }
    }

    correlation.calculate_energy_metrics();
    correlations.push_back(std::move(correlation));
  }

  return correlations;
}

std::vector<EnergyReading> EnergyCorrelator::filter_readings_by_time_range(
    const std::vector<EnergyReading>& readings,
    std::chrono::time_point<std::chrono::system_clock> start_time,
    std::chrono::time_point<std::chrono::system_clock> end_time) const {
  std::vector<EnergyReading> filtered;

  for (const auto& reading : readings) {
    if (reading.timestamp >= start_time && reading.timestamp <= end_time) {
      filtered.push_back(reading);
    }
  }

  return filtered;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

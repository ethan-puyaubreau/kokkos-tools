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
#include <algorithm>
#include <map>
#include <set>
#include <set>

namespace KokkosTools {
namespace PowerProfiler {

double EnergyCorrelator::calculate_energy_joules(
    const std::vector<EnergyReading>& readings,
    std::chrono::time_point<std::chrono::system_clock> start_time,
    std::chrono::time_point<std::chrono::system_clock> end_time,
    uint32_t device_id) {
  if (readings.empty()) {
    return 0.0;
  }

  // If no readings exactly in the time range, find the closest ones before and after
  auto duration_seconds = std::chrono::duration<double>(end_time - start_time).count();
  if (duration_seconds <= 0.0) {
    return 0.0;
  }

  // Filter readings in time range
  std::vector<EnergyReading> filtered;
  for (const auto& reading : readings) {
    if (reading.timestamp >= start_time && reading.timestamp <= end_time) {
      filtered.push_back(reading);
    }
  }

  // If we have no readings in the range, we need to interpolate from adjacent readings
  if (filtered.empty()) {
    // Find closest reading before start_time
    EnergyReading before;
    bool found_before = false;
    for (const auto& reading : readings) {
      if (reading.timestamp <= start_time && 
          (!found_before || reading.timestamp > before.timestamp)) {
        before = reading;
        found_before = true;
      }
    }

    // Find closest reading after end_time
    EnergyReading after;
    bool found_after = false;
    for (const auto& reading : readings) {
      if (reading.timestamp >= end_time && 
          (!found_after || reading.timestamp < after.timestamp)) {
        after = reading;
        found_after = true;
      }
    }

    // If we have both readings, interpolate
    if (found_before && found_after) {
      double total_power = 0.0;
      
      if (device_id == UINT32_MAX) {
        // Sum all device powers
        for (const auto& device : before.device_readings) {
          // Find corresponding device in after reading
          for (const auto& after_device : after.device_readings) {
            if (device.device_id == after_device.device_id) {
              double power = (device.power_watts + after_device.power_watts) / 2.0;
              total_power += power;
              break;
            }
          }
        }
      } else {
        // Only use the specified device
        double before_power = 0.0, after_power = 0.0;
        
        for (const auto& device : before.device_readings) {
          if (device.device_id == device_id) {
            before_power = device.power_watts;
            break;
          }
        }
        
        for (const auto& device : after.device_readings) {
          if (device.device_id == device_id) {
            after_power = device.power_watts;
            break;
          }
        }
        
        total_power = (before_power + after_power) / 2.0;
      }
      
      return total_power * duration_seconds;
    }
    
    // If we're missing one of the readings, return 0
    return 0.0;
  }
  
  // Calculate energy using the trapezoid rule for integration
  double energy = 0.0;
  std::map<uint32_t, double> prev_powers;
  std::chrono::time_point<std::chrono::system_clock> prev_time = start_time;
  bool first = true;
  
  // Initialize with powers at start_time (using first reading)
  if (!filtered.empty()) {
    for (const auto& device : filtered[0].device_readings) {
      if (device_id == UINT32_MAX || device.device_id == device_id) {
        prev_powers[device.device_id] = device.power_watts;
      }
    }
  }
  
  // Process all readings in chronological order
  for (const auto& reading : filtered) {
    if (first) {
      first = false;
      prev_time = reading.timestamp;
      continue;
    }
    
    // Calculate time segment in seconds
    double segment_duration = 
        std::chrono::duration<double>(reading.timestamp - prev_time).count();
    
    if (device_id == UINT32_MAX) {
      // Process all devices
      for (const auto& device : reading.device_readings) {
        double prev_power = 0.0;
        if (prev_powers.count(device.device_id)) {
          prev_power = prev_powers[device.device_id];
        }
        
        // Trapezoid rule: avg_power = (power1 + power2) / 2
        double avg_power = (prev_power + device.power_watts) / 2.0;
        energy += avg_power * segment_duration;
        
        // Update for next iteration
        prev_powers[device.device_id] = device.power_watts;
      }
    } else {
      // Process just the specified device
      double current_power = 0.0;
      for (const auto& device : reading.device_readings) {
        if (device.device_id == device_id) {
          current_power = device.power_watts;
          break;
        }
      }
      
      double prev_power = prev_powers.count(device_id) ? prev_powers[device_id] : 0.0;
      double avg_power = (prev_power + current_power) / 2.0;
      energy += avg_power * segment_duration;
      
      // Update for next iteration
      prev_powers[device_id] = current_power;
    }
    
    prev_time = reading.timestamp;
  }
  
  // Handle final segment if needed (from last reading to end_time)
  if (!filtered.empty()) {
    const auto& last_reading = filtered.back();
    if (last_reading.timestamp < end_time) {
      double segment_duration = 
          std::chrono::duration<double>(end_time - last_reading.timestamp).count();
      
      double total_power = 0.0;
      if (device_id == UINT32_MAX) {
        // Sum all device powers from the last reading
        for (const auto& device : last_reading.device_readings) {
          total_power += device.power_watts;
        }
      } else {
        // Use just the specified device
        for (const auto& device : last_reading.device_readings) {
          if (device.device_id == device_id) {
            total_power = device.power_watts;
            break;
          }
        }
      }
      
      energy += total_power * segment_duration;
    }
  }
  
  return energy;
}

void KernelEnergyCorrelation::calculate_energy_metrics(const KernelTiming& kernel) {
  if (energy_readings.empty()) {
    total_energy_joules = 0.0;
    average_power_watts = 0.0;
    energy_by_device.clear();
    return;
  }

  // Calculate total energy across all devices using interpolation
  total_energy_joules = EnergyCorrelator::calculate_energy_joules(
      energy_readings, kernel.start_time, kernel.end_time);
      
  // Calculate energy per device
  std::set<uint32_t> device_ids;
  for (const auto& reading : energy_readings) {
    for (const auto& device : reading.device_readings) {
      device_ids.insert(device.device_id);
    }
  }
  
  energy_by_device.clear();
  for (uint32_t device_id : device_ids) {
    energy_by_device[device_id] = EnergyCorrelator::calculate_energy_joules(
        energy_readings, kernel.start_time, kernel.end_time, device_id);
  }

  // Calculate average power
  auto duration_seconds = std::chrono::duration<double>(kernel.duration()).count();
  if (duration_seconds > 0.0) {
    average_power_watts = total_energy_joules / duration_seconds;
  } else {
    average_power_watts = 0.0;
  }
}

void RegionEnergyCorrelation::calculate_energy_metrics(const RegionTiming& region) {
  if (energy_readings.empty()) {
    total_energy_joules = 0.0;
    average_power_watts = 0.0;
    energy_by_device.clear();
    return;
  }

  // Calculate total energy across all devices using interpolation
  total_energy_joules = EnergyCorrelator::calculate_energy_joules(
      energy_readings, region.start_time, region.end_time);
      
  // Calculate energy per device
  std::set<uint32_t> device_ids;
  for (const auto& reading : energy_readings) {
    for (const auto& device : reading.device_readings) {
      device_ids.insert(device.device_id);
    }
  }
  
  energy_by_device.clear();
  for (uint32_t device_id : device_ids) {
    energy_by_device[device_id] = EnergyCorrelator::calculate_energy_joules(
        energy_readings, region.start_time, region.end_time, device_id);
  }

  // Calculate average power
  auto duration_seconds = std::chrono::duration<double>(region.duration()).count();
  if (duration_seconds > 0.0) {
    average_power_watts = total_energy_joules / duration_seconds;
  } else {
    average_power_watts = 0.0;
  }
}

void RegionEnergyCorrelation::add_kernel(uint64_t kernel_id) {
  kernel_ids.push_back(kernel_id);
}

std::vector<KernelEnergyCorrelation>
EnergyCorrelator::correlate_kernels_with_energy(
    const std::vector<KernelTiming>& kernels,
    const std::vector<EnergyReading>& energy_readings) const {
  std::vector<KernelEnergyCorrelation> correlations;
  correlations.reserve(kernels.size());

  for (const auto& kernel : kernels) {
    KernelEnergyCorrelation correlation(kernel.id);

    correlation.energy_readings = filter_readings_by_time_range(
        energy_readings, kernel.start_time, kernel.end_time);

    correlation.calculate_energy_metrics(kernel);
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
    RegionEnergyCorrelation correlation(region.id);

    correlation.energy_readings = filter_readings_by_time_range(
        energy_readings, region.start_time, region.end_time);

    // Add kernels that fall within this region's time range
    for (const auto& kernel : kernels) {
      if (kernel.start_time >= region.start_time &&
          kernel.end_time <= region.end_time) {
        correlation.add_kernel(kernel.id);
      }
    }

    correlation.calculate_energy_metrics(region);
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

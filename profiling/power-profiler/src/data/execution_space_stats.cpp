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

#include "execution_space_stats.hpp"
#include <algorithm>
#include <limits>

namespace KokkosTools {
namespace PowerProfiler {

std::map<ExecutionSpace, ExecutionSpaceStats> 
ExecutionSpaceAnalyzer::analyze_kernel_timings(const std::vector<KernelTiming>& timings) {
  std::map<ExecutionSpace, ExecutionSpaceStats> stats_map;
  
  for (const auto& timing : timings) {
    ExecutionSpace space = timing.execution_space;
    
    // Initialize stats if this is the first kernel for this execution space
    if (stats_map.find(space) == stats_map.end()) {
      stats_map[space] = ExecutionSpaceStats(space);
      stats_map[space].min_duration_ms = std::numeric_limits<int64_t>::max();
      stats_map[space].max_duration_ms = std::numeric_limits<int64_t>::min();
    }
    
    auto& stats = stats_map[space];
    int64_t duration = timing.duration_ms_value();
    
    // Update statistics
    stats.kernel_count++;
    stats.total_duration_ms += duration;
    stats.min_duration_ms = std::min(stats.min_duration_ms, duration);
    stats.max_duration_ms = std::max(stats.max_duration_ms, duration);
    stats.kernel_names.push_back(timing.name);
  }
  
  // Calculate averages
  for (auto& pair : stats_map) {
    auto& stats = pair.second;
    if (stats.kernel_count > 0) {
      stats.average_duration_ms = static_cast<double>(stats.total_duration_ms) / stats.kernel_count;
    }
  }
  
  return stats_map;
}

std::map<ExecutionSpace, ExecutionSpaceStats> 
ExecutionSpaceAnalyzer::analyze_kernel_energy(const std::vector<KernelEnergyCorrelation>& correlations) {
  std::map<ExecutionSpace, ExecutionSpaceStats> stats_map;
  
  // Since correlations only have kernel IDs, we need the actual kernel timings
  // This method should be called with both timings and correlations
  // For now, we'll create a minimal implementation that just uses the energy data
  
  for (const auto& corr : correlations) {
    // We can't determine execution space from correlation alone
    // This method needs to be redesigned or called differently
    // For now, just aggregate all energy data under UNKNOWN
    ExecutionSpace space = ExecutionSpace::UNKNOWN;
    
    // Initialize stats if this is the first kernel for this execution space
    if (stats_map.find(space) == stats_map.end()) {
      stats_map[space] = ExecutionSpaceStats(space);
      stats_map[space].min_power_watts = std::numeric_limits<double>::max();
      stats_map[space].max_power_watts = std::numeric_limits<double>::lowest();
    }
    
    auto& stats = stats_map[space];
    
    // Update energy statistics
    stats.kernel_count++;
    stats.total_energy_joules += corr.total_energy_joules;
    stats.min_power_watts = std::min(stats.min_power_watts, corr.average_power_watts);
    stats.max_power_watts = std::max(stats.max_power_watts, corr.average_power_watts);
  }
  
  // Calculate averages
  for (auto& pair : stats_map) {
    auto& stats = pair.second;
    if (stats.kernel_count > 0) {
      // We don't have duration info in correlations, so we can't calculate proper average power
      stats.average_power_watts = stats.total_energy_joules / stats.kernel_count; // Rough approximation
    }
  }
  
  return stats_map;
}

std::map<ExecutionSpace, ExecutionSpaceStats> 
ExecutionSpaceAnalyzer::merge_timing_and_energy_stats(
    const std::map<ExecutionSpace, ExecutionSpaceStats>& timing_stats,
    const std::map<ExecutionSpace, ExecutionSpaceStats>& energy_stats) {
  
  std::map<ExecutionSpace, ExecutionSpaceStats> merged_stats = timing_stats;
  
  for (const auto& energy_pair : energy_stats) {
    ExecutionSpace space = energy_pair.first;
    const auto& energy_stats_ref = energy_pair.second;
    
    if (merged_stats.find(space) != merged_stats.end()) {
      auto& stats = merged_stats[space];
      stats.total_energy_joules = energy_stats_ref.total_energy_joules;
      stats.average_power_watts = energy_stats_ref.average_power_watts;
      stats.min_power_watts = energy_stats_ref.min_power_watts;
      stats.max_power_watts = energy_stats_ref.max_power_watts;
    } else {
      merged_stats[space] = energy_stats_ref;
    }
  }
  
  return merged_stats;
}

std::pair<ExecutionSpaceStats, ExecutionSpaceStats>
ExecutionSpaceAnalyzer::create_host_vs_device_summary(
    const std::map<ExecutionSpace, ExecutionSpaceStats>& stats) {
  
  ExecutionSpaceStats host_summary;
  ExecutionSpaceStats device_summary;
  
  host_summary.space_name = "HOST_COMBINED";
  host_summary.is_host = true;
  host_summary.is_device = false;
  
  device_summary.space_name = "DEVICE_COMBINED";
  device_summary.is_host = false;
  device_summary.is_device = true;
  
  // Initialize min/max values
  host_summary.min_duration_ms = std::numeric_limits<int64_t>::max();
  host_summary.max_duration_ms = std::numeric_limits<int64_t>::min();
  host_summary.min_power_watts = std::numeric_limits<double>::max();
  host_summary.max_power_watts = std::numeric_limits<double>::lowest();
  
  device_summary.min_duration_ms = std::numeric_limits<int64_t>::max();
  device_summary.max_duration_ms = std::numeric_limits<int64_t>::min();
  device_summary.min_power_watts = std::numeric_limits<double>::max();
  device_summary.max_power_watts = std::numeric_limits<double>::lowest();
  
  for (const auto& pair : stats) {
    const auto& space_stats = pair.second;
    
    if (space_stats.is_host) {
      host_summary.kernel_count += space_stats.kernel_count;
      host_summary.total_duration_ms += space_stats.total_duration_ms;
      host_summary.total_energy_joules += space_stats.total_energy_joules;
      
      if (space_stats.kernel_count > 0) {
        host_summary.min_duration_ms = std::min(host_summary.min_duration_ms, space_stats.min_duration_ms);
        host_summary.max_duration_ms = std::max(host_summary.max_duration_ms, space_stats.max_duration_ms);
        host_summary.min_power_watts = std::min(host_summary.min_power_watts, space_stats.min_power_watts);
        host_summary.max_power_watts = std::max(host_summary.max_power_watts, space_stats.max_power_watts);
      }
      
      host_summary.kernel_names.insert(host_summary.kernel_names.end(),
                                       space_stats.kernel_names.begin(),
                                       space_stats.kernel_names.end());
    } else if (space_stats.is_device) {
      device_summary.kernel_count += space_stats.kernel_count;
      device_summary.total_duration_ms += space_stats.total_duration_ms;
      device_summary.total_energy_joules += space_stats.total_energy_joules;
      
      if (space_stats.kernel_count > 0) {
        device_summary.min_duration_ms = std::min(device_summary.min_duration_ms, space_stats.min_duration_ms);
        device_summary.max_duration_ms = std::max(device_summary.max_duration_ms, space_stats.max_duration_ms);
        device_summary.min_power_watts = std::min(device_summary.min_power_watts, space_stats.min_power_watts);
        device_summary.max_power_watts = std::max(device_summary.max_power_watts, space_stats.max_power_watts);
      }
      
      device_summary.kernel_names.insert(device_summary.kernel_names.end(),
                                          space_stats.kernel_names.begin(),
                                          space_stats.kernel_names.end());
    }
  }
  
  // Calculate averages
  if (host_summary.kernel_count > 0) {
    host_summary.average_duration_ms = static_cast<double>(host_summary.total_duration_ms) / host_summary.kernel_count;
    host_summary.average_power_watts = host_summary.total_energy_joules / 
        (static_cast<double>(host_summary.total_duration_ms) / 1000.0);
  }
  
  if (device_summary.kernel_count > 0) {
    device_summary.average_duration_ms = static_cast<double>(device_summary.total_duration_ms) / device_summary.kernel_count;
    device_summary.average_power_watts = device_summary.total_energy_joules / 
        (static_cast<double>(device_summary.total_duration_ms) / 1000.0);
  }
  
  return std::make_pair(host_summary, device_summary);
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

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

#include "timing_data.hpp"
#include "correlation.hpp"
#include <map>
#include <vector>

namespace KokkosTools {
namespace PowerProfiler {

/**
 * @brief Statistics aggregated by execution space
 */
struct ExecutionSpaceStats {
  ExecutionSpace space;
  std::string space_name;
  bool is_host;
  bool is_device;
  
  // Kernel statistics
  size_t kernel_count = 0;
  int64_t total_duration_ms = 0;
  int64_t min_duration_ms = 0;
  int64_t max_duration_ms = 0;
  double average_duration_ms = 0.0;
  
  // Energy statistics (if available)
  double total_energy_joules = 0.0;
  double average_power_watts = 0.0;
  double min_power_watts = 0.0;
  double max_power_watts = 0.0;
  
  // List of kernel names for reference
  std::vector<std::string> kernel_names;
  
  ExecutionSpaceStats() = default;
  ExecutionSpaceStats(ExecutionSpace exec_space)
      : space(exec_space), 
        space_name(execution_space_to_string(exec_space)),
        is_host(is_host_execution_space(exec_space)),
        is_device(is_device_execution_space(exec_space)) {}
};

/**
 * @brief Utility class to compute execution space statistics
 */
class ExecutionSpaceAnalyzer {
public:
  /**
   * @brief Compute kernel statistics grouped by execution space
   */
  static std::map<ExecutionSpace, ExecutionSpaceStats> 
  analyze_kernel_timings(const std::vector<KernelTiming>& timings);
  
  /**
   * @brief Compute energy statistics grouped by execution space
   */
  static std::map<ExecutionSpace, ExecutionSpaceStats> 
  analyze_kernel_energy(const std::vector<KernelEnergyCorrelation>& correlations);
  
  /**
   * @brief Merge timing and energy statistics by execution space
   */
  static std::map<ExecutionSpace, ExecutionSpaceStats> 
  merge_timing_and_energy_stats(
      const std::map<ExecutionSpace, ExecutionSpaceStats>& timing_stats,
      const std::map<ExecutionSpace, ExecutionSpaceStats>& energy_stats);
  
  /**
   * @brief Create a summary comparing host vs device execution
   */
  static std::pair<ExecutionSpaceStats, ExecutionSpaceStats>
  create_host_vs_device_summary(const std::map<ExecutionSpace, ExecutionSpaceStats>& stats);
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

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

#include "../data/timing_data.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace KokkosTools {
namespace PowerProfiler {

class TimingManager {
 public:
  void begin_kernel(uint64_t kernel_id, const std::string& name,
                    KernelType type);
  void end_kernel(uint64_t kernel_id);

  void push_region(const std::string& name);
  void pop_region();

  const std::vector<KernelTiming>& get_kernel_timings() const {
    return completed_kernels_;
  }
  const std::vector<RegionTiming>& get_region_timings() const {
    return completed_regions_;
  }

  // Additional utility methods
  void clear_completed_data();
  size_t get_active_kernel_count() const;
  size_t get_active_region_count() const;
  std::vector<std::string> get_active_region_names() const;

 private:
  std::chrono::time_point<std::chrono::steady_clock> get_current_time() const;

  std::unordered_map<uint64_t, KernelTiming> active_kernels_;
  std::vector<KernelTiming> completed_kernels_;

  std::vector<RegionTiming> active_regions_;
  std::vector<RegionTiming> completed_regions_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

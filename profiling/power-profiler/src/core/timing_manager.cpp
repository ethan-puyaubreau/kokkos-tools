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

#include "timing_manager.hpp"
#include <iostream>

namespace KokkosTools {
namespace PowerProfiler {

void TimingManager::begin_kernel(uint64_t kernel_id, const std::string& name,
                                 KernelType type) {
  auto current_time = get_current_time();

  if (active_kernels_.find(kernel_id) != active_kernels_.end()) {
    std::cerr << "Warning: Kernel ID " << kernel_id
              << " already active. Overwriting." 
              << "\n";
  }

  active_kernels_[kernel_id] = KernelTiming(name, type, current_time);
}

void TimingManager::end_kernel(uint64_t kernel_id) {
  auto it = active_kernels_.find(kernel_id);
  if (it != active_kernels_.end()) {
    it->second.end_time = get_current_time();
    completed_kernels_.push_back(std::move(it->second));
    active_kernels_.erase(it);
  } else {
    std::cerr << "Warning: Attempting to end non-existent kernel ID "
              << kernel_id << "\n";
  }
}

void TimingManager::push_region(const std::string& name) {
  auto current_time = get_current_time();
  active_regions_.emplace_back(name, current_time);
}

void TimingManager::pop_region() {
  if (!active_regions_.empty()) {
    active_regions_.back().end_time = get_current_time();
    completed_regions_.push_back(std::move(active_regions_.back()));
    active_regions_.pop_back();
  } else {
    std::cerr << "Warning: Attempting to pop region from empty stack" 
    << "\n";
  }
}

std::chrono::time_point<std::chrono::steady_clock>
TimingManager::get_current_time() const {
  return std::chrono::steady_clock::now();
}

void TimingManager::clear_completed_data() {
  completed_kernels_.clear();
  completed_regions_.clear();
}

size_t TimingManager::get_active_kernel_count() const {
  return active_kernels_.size();
}

size_t TimingManager::get_active_region_count() const {
  return active_regions_.size();
}

std::vector<std::string> TimingManager::get_active_region_names() const {
  std::vector<std::string> names;
  names.reserve(active_regions_.size());
  for (const auto& region : active_regions_) {
    names.push_back(region.name);
  }
  return names;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

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

#include "power_monitor.hpp"
#include "../core/timing_manager.hpp"
#include "../core/config.hpp"
#include "../output/output_handler.hpp"
#include "../data/correlation.hpp"
#include <memory>

namespace KokkosTools {
namespace PowerProfiler {

class PowerProfilerCore {
 public:
  PowerProfilerCore(ProfilerConfig& config);
  ~PowerProfilerCore() = default;

  bool initialize();
  void finalize();

  void begin_kernel(uint64_t kernel_id, const std::string& name,
                    KernelType type, uint32_t device_id);
  void end_kernel(uint64_t kernel_id);

  void push_region(const std::string& name);
  void pop_region();

  bool is_initialized() const { return initialization_successful_; }

 private:
  bool setup_energy_monitoring();
  void setup_output_handler();
  void perform_analysis_and_output();

  ProfilerConfig& config_;
  std::unique_ptr<PowerMonitor> power_monitor_;
  std::unique_ptr<TimingManager> timing_manager_;
  std::unique_ptr<OutputHandler> output_handler_;
  std::unique_ptr<EnergyCorrelator> correlator_;

  bool initialization_successful_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

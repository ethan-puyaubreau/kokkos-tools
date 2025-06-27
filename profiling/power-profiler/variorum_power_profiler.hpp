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

#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <thread>
#include <atomic>
#include <fstream>
#include <map> // Added for std::map<uint32_t, double>

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

namespace KokkosTools {
namespace PowerProfiler {

enum class KernelType { FOR, SCAN, REDUCE };

struct EnergyReading {
  std::chrono::time_point<std::chrono::steady_clock> timestamp;
  // Changed to store power readings directly, indexed by device_id
  std::map<uint32_t, double> gpu_power_watts; 
};

struct KernelTiming {
  uint64_t kernel_id;
  std::string name;
  KernelType type;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> end_time;
  std::chrono::nanoseconds duration;
};

struct RegionTiming {
  std::string name;
  std::chrono::time_point<std::chrono::steady_clock> start_time;
  std::chrono::time_point<std::chrono::steady_clock> end_time;
  std::chrono::nanoseconds duration;
};

/**
 * Simplified Variorum-specific power profiler with integrated timing
 */
class VariorumPowerProfiler {
public:
  VariorumPowerProfiler();
  ~VariorumPowerProfiler();

  bool initialize();
  void finalize();

  void begin_kernel(uint64_t kernel_id, const std::string& name, KernelType type);
  void end_kernel(uint64_t kernel_id);

  void push_region(const std::string& name);
  void pop_region();

  bool is_initialized() const { return initialized_; }

private:
  // Variorum energy monitoring
  struct JsonDeleter {
    void operator()(json_t* json) const {
      if (json) json_decref(json);
    }
  };
  using unique_json_ptr = std::unique_ptr<json_t, JsonDeleter>;

  struct CFreeDeleter {
    void operator()(char* ptr) const {
      if (ptr) free(ptr);
    }
  };
  using unique_cstring = std::unique_ptr<char, CFreeDeleter>;

  bool initialize_variorum();
  unique_json_ptr get_variorum_json_data() const;
  EnergyReading get_current_energy_reading() const;
  std::vector<uint32_t> get_available_devices() const;

  // Background monitoring
  void monitoring_thread_function();
  void start_monitoring();
  void stop_monitoring();

  // Output generation
  void generate_outputs();
  void output_to_console();
  void output_to_json();
  void output_to_csv();

  // Timing utilities
  std::chrono::time_point<std::chrono::steady_clock> get_current_time() const;
  std::chrono::nanoseconds get_time_relative_to_first_measurement(
      const std::chrono::time_point<std::chrono::steady_clock>& timepoint) const;

  // Configuration
  std::chrono::microseconds monitor_interval_{20000}; // 20ms
  std::string output_file_path_{"power_profile_output"};
  
  // State
  bool initialized_{false};
  std::vector<uint32_t> available_devices_;
  std::chrono::time_point<std::chrono::steady_clock> first_measurement_time_;
  bool first_measurement_recorded_{false};

  // Monitoring thread
  std::atomic<bool> monitoring_active_{false};
  std::unique_ptr<std::thread> monitoring_thread_;

  // Data storage
  std::vector<EnergyReading> energy_readings_;
  std::vector<KernelTiming> completed_kernels_;
  std::vector<RegionTiming> completed_regions_;

  // Active tracking
  std::unordered_map<uint64_t, KernelTiming> active_kernels_;
  std::vector<RegionTiming> active_regions_;
};

} // namespace PowerProfiler
} // namespace KokkosTools
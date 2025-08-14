//@HEADER
// ************************************************************************
//
//                        Kokkos Energy Consumption Profiler
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

/**
 * @file kp_nvml_energy_consumption.cpp
 * @brief Kokkos Energy Consumption Profiler Tool using NVML.
 *
 * This tool measures energy consumption by tracking the cumulative energy
 * values from NVML at the beginning and end of kernels, regions, and deep
 * copies. It does not use a background daemon since the energy consumption is a
 * cumulative counter that can be read directly when events occur.
 */

#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <stack>
#include <cstring>
#include <mutex>
#include <iomanip>

#include "kp_core.hpp"
#include "../provider/provider_nvml.hpp"
#include "../common/filename_prefix.hpp"
#include "../common/timer_system.hpp"
#include "../common/error_handling.hpp"

namespace KokkosTools {
namespace EnergyConsumption {

using EnergyProfiler::ErrorCode;
using EnergyProfiler::NVMLProvider;
using EnergyProfiler::Result;

// --- Global State for the Profiler ---
static constexpr const char* COMPONENT_NAME = "EnergyConsumption";
static std::unique_ptr<NVMLProvider> g_nvml_provider;

// Timer tool for kernel and region timing
static Timer::KernelTimerTool g_timer;

static size_t g_device_count = 0;
static std::chrono::high_resolution_clock::time_point g_start_time;

// Energy tracking structures
struct EnergySnapshot {
  std::chrono::high_resolution_clock::time_point timestamp;
  std::vector<double>
      device_energies_joules;  // Energy for each device in Joules
};

struct KernelEnergyRecord {
  std::string name;
  uint32_t devID;
  uint64_t kID;
  EnergySnapshot start_energy;
  EnergySnapshot end_energy;
  double duration_seconds;
  std::vector<double> energy_consumed_joules;  // Per device
};

struct RegionEnergyRecord {
  std::string name;
  EnergySnapshot start_energy;
  EnergySnapshot end_energy;
  double duration_seconds;
  std::vector<double> energy_consumed_joules;  // Per device
};

struct DeepCopyEnergyRecord {
  std::string dst_name;
  std::string src_name;
  uint64_t size;
  EnergySnapshot start_energy;
  EnergySnapshot end_energy;
  double duration_seconds;
  std::vector<double> energy_consumed_joules;  // Per device
};

// Storage for energy records
static std::vector<KernelEnergyRecord> g_kernel_energy_records;
static std::vector<RegionEnergyRecord> g_region_energy_records;
static std::vector<DeepCopyEnergyRecord> g_deep_copy_energy_records;
static std::mutex g_energy_mutex;

// Stack for nested regions
static std::stack<std::pair<std::string, EnergySnapshot>> g_region_stack;

// Maps for tracking active kernels/deep copies
static std::unordered_map<uint64_t, KernelEnergyRecord> g_active_kernels;
static std::pair<bool, DeepCopyEnergyRecord> g_active_deep_copy = {false, {}};

/**
 * @brief Captures a snapshot of current energy consumption for all devices.
 */
EnergySnapshot capture_energy_snapshot() {
  EnergySnapshot snapshot;
  snapshot.timestamp = std::chrono::high_resolution_clock::now();
  snapshot.device_energies_joules.reserve(g_device_count);

  if (!g_nvml_provider || !g_nvml_provider->is_initialized()) {
    // Fill with invalid values
    for (size_t i = 0; i < g_device_count; ++i) {
      snapshot.device_energies_joules.push_back(-1.0);
    }
    ENERGY_PROFILER_LOG_WARNING(
        COMPONENT_NAME,
        "Provider not initialized, using invalid energy values");
    return snapshot;
  }

  // Collect energy for each device
  for (size_t i = 0; i < g_device_count; ++i) {
    double energy = 0.0;
    Result result = g_nvml_provider->get_current_energy_consumption(i, energy);
    if (result) {
      snapshot.device_energies_joules.push_back(energy);
    } else {
      snapshot.device_energies_joules.push_back(-1.0);
      ENERGY_PROFILER_LOG_WARNING(
          COMPONENT_NAME, "Failed to get energy for device " +
                              std::to_string(i) + ": " + result.message);
    }
  }

  return snapshot;
}

/**
 * @brief Calculates energy consumed between two snapshots.
 */
std::vector<double> calculate_energy_delta(const EnergySnapshot& start,
                                           const EnergySnapshot& end) {
  std::vector<double> delta(g_device_count, 0.0);

  for (size_t i = 0; i < g_device_count; ++i) {
    if (i < start.device_energies_joules.size() &&
        i < end.device_energies_joules.size() &&
        start.device_energies_joules[i] >= 0 &&
        end.device_energies_joules[i] >= 0) {
      delta[i] =
          end.device_energies_joules[i] - start.device_energies_joules[i];
      // Handle potential counter reset (though rare)
      if (delta[i] < 0) {
        delta[i] = 0;  // Reset occurred, use 0 as approximation
      }
    } else {
      delta[i] = -1.0;  // Invalid measurement
    }
  }

  return delta;
}

/**
 * @brief Calculates duration in seconds between two snapshots.
 */
double calculate_duration_seconds(const EnergySnapshot& start,
                                  const EnergySnapshot& end) {
  return std::chrono::duration<double>(end.timestamp - start.timestamp).count();
}

/**
 * @brief Exports energy consumption data to CSV file with error handling
 */
Result export_energy_consumption_csv(const std::string& filename) {
  std::ofstream file(filename);
  if (!file.is_open()) {
    std::string error_msg = "Unable to open file " + filename + " for writing";
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::FILE_WRITE_FAILED, error_msg);
  }

  try {
    // Write header
    file << "type,name,duration_seconds";
    for (size_t i = 0; i < g_device_count; ++i) {
      file << ",device_" << i << "_energy_joules";
    }
    file << "\n";

    // Write kernels
    for (const auto& record : g_kernel_energy_records) {
      file << "kernel," << record.name << "," << record.duration_seconds;
      for (size_t i = 0; i < g_device_count; ++i) {
        if (i < record.energy_consumed_joules.size()) {
          file << "," << record.energy_consumed_joules[i];
        } else {
          file << ",-1";
        }
      }
      file << "\n";
    }

    // Write regions
    for (const auto& record : g_region_energy_records) {
      file << "region," << record.name << "," << record.duration_seconds;
      for (size_t i = 0; i < g_device_count; ++i) {
        if (i < record.energy_consumed_joules.size()) {
          file << "," << record.energy_consumed_joules[i];
        } else {
          file << ",-1";
        }
      }
      file << "\n";
    }

    // Write deep copies
    for (const auto& record : g_deep_copy_energy_records) {
      std::string name = record.src_name + "_to_" + record.dst_name + "_size_" +
                         std::to_string(record.size);
      file << "deepcopy," << name << "," << record.duration_seconds;
      for (size_t i = 0; i < g_device_count; ++i) {
        if (i < record.energy_consumed_joules.size()) {
          file << "," << record.energy_consumed_joules[i];
        } else {
          file << ",-1";
        }
      }
      file << "\n";
    }

    file.close();

    if (file.fail()) {
      std::string error_msg = "Failed to write data to file " + filename;
      ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
      return Result(ErrorCode::FILE_WRITE_FAILED, error_msg);
    }

    ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME,
                             "Energy consumption data exported to " + filename);
    return Result(ErrorCode::SUCCESS);

  } catch (const std::exception& e) {
    std::string error_msg =
        "Exception while writing to file " + filename + ": " + e.what();
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::FILE_WRITE_FAILED, error_msg);
  }
}

void print_energy_summary() {
  std::cout << "\n==== Energy Consumption Profile Summary ====\n";
  std::cout << std::fixed << std::setprecision(4);

  // Calculate total energy per device
  std::vector<double> total_kernel_energy(g_device_count, 0.0);
  std::vector<double> total_region_energy(g_device_count, 0.0);
  std::vector<double> total_deepcopy_energy(g_device_count, 0.0);

  for (const auto& record : g_kernel_energy_records) {
    for (size_t i = 0;
         i < g_device_count && i < record.energy_consumed_joules.size(); ++i) {
      if (record.energy_consumed_joules[i] >= 0) {
        total_kernel_energy[i] += record.energy_consumed_joules[i];
      }
    }
  }

  for (const auto& record : g_region_energy_records) {
    for (size_t i = 0;
         i < g_device_count && i < record.energy_consumed_joules.size(); ++i) {
      if (record.energy_consumed_joules[i] >= 0) {
        total_region_energy[i] += record.energy_consumed_joules[i];
      }
    }
  }

  for (const auto& record : g_deep_copy_energy_records) {
    for (size_t i = 0;
         i < g_device_count && i < record.energy_consumed_joules.size(); ++i) {
      if (record.energy_consumed_joules[i] >= 0) {
        total_deepcopy_energy[i] += record.energy_consumed_joules[i];
      }
    }
  }

  std::cout << "Number of Kernels:         " << g_kernel_energy_records.size()
            << "\n";
  std::cout << "Number of Regions:         " << g_region_energy_records.size()
            << "\n";
  std::cout << "Number of Deep Copies:     "
            << g_deep_copy_energy_records.size() << "\n";
  std::cout << "Number of Devices:         " << g_device_count << "\n";
  std::cout << "--------------------------------------------\n";

  for (size_t dev = 0; dev < g_device_count; ++dev) {
    std::string device_name = "Unknown Device";
    if (g_nvml_provider && g_nvml_provider->is_initialized()) {
      device_name = g_nvml_provider->get_device_name(dev);
    }

    std::cout << "Device " << dev << " (" << device_name << "):\n";
    std::cout << "  Total Kernel Energy:       " << total_kernel_energy[dev]
              << " J\n";
    std::cout << "  Total Region Energy:       " << total_region_energy[dev]
              << " J\n";
    std::cout << "  Total Deep Copy Energy:    " << total_deepcopy_energy[dev]
              << " J\n";
    std::cout << "  Total Energy:              "
              << (total_kernel_energy[dev] + total_region_energy[dev] +
                  total_deepcopy_energy[dev])
              << " J\n";
    std::cout << "--------------------------------------------\n";
  }
}

// --- Kokkos Profiling Hooks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "Initializing...");

  // Initialize the timer tool
  g_timer.init_library(loadSeq, interfaceVer, devInfoCount, deviceInfo);

  g_nvml_provider    = std::make_unique<NVMLProvider>();
  Result init_result = g_nvml_provider->initialize();
  if (!init_result) {
    std::string error_msg =
        "Failed to initialize NVML provider: " + init_result.message;
    ENERGY_PROFILER_LOG_ERROR(
        COMPONENT_NAME, error_msg + ". Energy consumption profiling disabled.");
    g_nvml_provider.reset();  // Release the provider
    return;
  }

  g_device_count = g_nvml_provider->get_device_count();
  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "NVML provider initialized with " +
                                               std::to_string(g_device_count) +
                                               " device(s)");

  // Print device information
  for (size_t i = 0; i < g_device_count; ++i) {
    ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME,
                             "Device " + std::to_string(i) + ": " +
                                 g_nvml_provider->get_device_name(i));
  }

  g_start_time = std::chrono::high_resolution_clock::now();
  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME,
                           "Energy consumption monitoring initialized");
}

void kokkosp_finalize_library() {
  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "Finalizing...");

  // Finalize the timer
  g_timer.finalize_library();

  auto end_time = std::chrono::high_resolution_clock::now();
  auto total_duration_s =
      std::chrono::duration<double>(end_time - g_start_time).count();

  ENERGY_PROFILER_LOG_INFO(
      COMPONENT_NAME,
      "Total Monitoring Duration: " + std::to_string(total_duration_s) + " s");

  print_energy_summary();

  std::string prefix = generate_prefix();

  // Export energy data
  std::string csv_filename = prefix + "_nvml_energy_consumption.csv";
  ENERGY_PROFILER_LOG_INFO(
      COMPONENT_NAME, "Exporting energy consumption data to " + csv_filename);
  Result export_result = export_energy_consumption_csv(csv_filename);
  if (!export_result) {
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, "Failed to export energy data: " +
                                                  export_result.message);
  }

  // Export timing data
  const auto& kernels = g_timer.get_kernel_timings();
  Timer::print_kernels_summary(kernels);
  Timer::export_kernels_csv(kernels, prefix + "_kernels.csv");

  const auto& regions = g_timer.get_region_timings();
  Timer::print_regions_summary(regions);
  Timer::export_regions_csv(regions, prefix + "_regions.csv");

  const auto& deepcopies = g_timer.get_deep_copy_timings();
  Timer::print_deepcopies_summary(deepcopies);
  Timer::export_deepcopies_csv(deepcopies, prefix + "_deepcopies.csv");

  if (g_nvml_provider) {
    g_nvml_provider->finalize();
    ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "NVML provider finalized");
  }
}

// --- Hook Implementations with Timer and Energy Integration ---
void kokkosp_begin_parallel_for(const char* name, uint32_t devID,
                                uint64_t* kID) {
  g_timer.begin_parallel_for(name, devID, *kID);

  // Capture energy snapshot
  KernelEnergyRecord record;
  record.name         = name;
  record.devID        = devID;
  record.kID          = *kID;
  record.start_energy = capture_energy_snapshot();

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  g_active_kernels[*kID] = record;
}

void kokkosp_end_parallel_for(uint64_t kID) {
  g_timer.end_parallel_for(kID);

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  auto it = g_active_kernels.find(kID);
  if (it != g_active_kernels.end()) {
    it->second.end_energy       = capture_energy_snapshot();
    it->second.duration_seconds = calculate_duration_seconds(
        it->second.start_energy, it->second.end_energy);
    it->second.energy_consumed_joules =
        calculate_energy_delta(it->second.start_energy, it->second.end_energy);

    g_kernel_energy_records.push_back(it->second);
    g_active_kernels.erase(it);
  }
}

void kokkosp_begin_parallel_scan(const char* name, uint32_t devID,
                                 uint64_t* kID) {
  g_timer.begin_parallel_scan(name, devID, kID);

  KernelEnergyRecord record;
  record.name         = name;
  record.devID        = devID;
  record.kID          = *kID;
  record.start_energy = capture_energy_snapshot();

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  g_active_kernels[*kID] = record;
}

void kokkosp_end_parallel_scan(uint64_t kID) {
  g_timer.end_parallel_scan(kID);

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  auto it = g_active_kernels.find(kID);
  if (it != g_active_kernels.end()) {
    it->second.end_energy       = capture_energy_snapshot();
    it->second.duration_seconds = calculate_duration_seconds(
        it->second.start_energy, it->second.end_energy);
    it->second.energy_consumed_joules =
        calculate_energy_delta(it->second.start_energy, it->second.end_energy);

    g_kernel_energy_records.push_back(it->second);
    g_active_kernels.erase(it);
  }
}

void kokkosp_begin_parallel_reduce(const char* name, uint32_t devID,
                                   uint64_t* kID) {
  g_timer.begin_parallel_reduce(name, devID, kID);

  KernelEnergyRecord record;
  record.name         = name;
  record.devID        = devID;
  record.kID          = *kID;
  record.start_energy = capture_energy_snapshot();

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  g_active_kernels[*kID] = record;
}

void kokkosp_end_parallel_reduce(uint64_t kID) {
  g_timer.end_parallel_reduce(kID);

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  auto it = g_active_kernels.find(kID);
  if (it != g_active_kernels.end()) {
    it->second.end_energy       = capture_energy_snapshot();
    it->second.duration_seconds = calculate_duration_seconds(
        it->second.start_energy, it->second.end_energy);
    it->second.energy_consumed_joules =
        calculate_energy_delta(it->second.start_energy, it->second.end_energy);

    g_kernel_energy_records.push_back(it->second);
    g_active_kernels.erase(it);
  }
}

void kokkosp_push_profile_region(const char* regionName) {
  g_timer.push_profile_region(regionName);

  EnergySnapshot snapshot = capture_energy_snapshot();
  g_region_stack.push({std::string(regionName), snapshot});
}

void kokkosp_pop_profile_region() {
  g_timer.pop_profile_region();

  if (!g_region_stack.empty()) {
    auto [name, start_energy] = g_region_stack.top();
    g_region_stack.pop();

    RegionEnergyRecord record;
    record.name         = name;
    record.start_energy = start_energy;
    record.end_energy   = capture_energy_snapshot();
    record.duration_seconds =
        calculate_duration_seconds(record.start_energy, record.end_energy);
    record.energy_consumed_joules =
        calculate_energy_delta(record.start_energy, record.end_energy);

    std::lock_guard<std::mutex> lock(g_energy_mutex);
    g_region_energy_records.push_back(record);
  }
}

void kokkosp_begin_deep_copy(Kokkos::Tools::SpaceHandle dst_handle,
                             const char* dst_name, const void* dst_ptr,
                             Kokkos::Tools::SpaceHandle src_handle,
                             const char* src_name, const void* src_ptr,
                             uint64_t size) {
  g_timer.begin_deep_copy(dst_handle, dst_name, dst_ptr, src_handle, src_name,
                          src_ptr, size);

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  if (!g_active_deep_copy.first) {
    g_active_deep_copy.second.dst_name     = dst_name ? dst_name : "unknown";
    g_active_deep_copy.second.src_name     = src_name ? src_name : "unknown";
    g_active_deep_copy.second.size         = size;
    g_active_deep_copy.second.start_energy = capture_energy_snapshot();
    g_active_deep_copy.first               = true;
  }
}

void kokkosp_end_deep_copy() {
  g_timer.end_deep_copy();

  std::lock_guard<std::mutex> lock(g_energy_mutex);
  if (g_active_deep_copy.first) {
    g_active_deep_copy.second.end_energy = capture_energy_snapshot();
    g_active_deep_copy.second.duration_seconds =
        calculate_duration_seconds(g_active_deep_copy.second.start_energy,
                                   g_active_deep_copy.second.end_energy);
    g_active_deep_copy.second.energy_consumed_joules =
        calculate_energy_delta(g_active_deep_copy.second.start_energy,
                               g_active_deep_copy.second.end_energy);

    g_deep_copy_energy_records.push_back(g_active_deep_copy.second);
    g_active_deep_copy.first = false;
  }
}

}  // namespace EnergyConsumption
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::EnergyConsumption;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)
EXPOSE_BEGIN_DEEP_COPY(impl::kokkosp_begin_deep_copy)
EXPOSE_END_DEEP_COPY(impl::kokkosp_end_deep_copy)

}  // extern "C"

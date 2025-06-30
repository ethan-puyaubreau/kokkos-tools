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

/**
 * Kokkos NVML Power Profiler
 * Simple Kokkos profiling tool that monitors GPU power consumption using NVML
 * Polls nvmlDeviceGetPowerUsage() every 100ms in a background thread
 * Calculates energy consumption by numerical integration of power over time
 */

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <limits>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <condition_variable>
#include <inttypes.h>

// NVML header
#include <nvml.h>

// Kokkos Profiling API headers - use same approach as simple timer
#include "kp_core.hpp"

namespace KokkosTools {
namespace NVMLPowerProfiler {

// --- Global variables for NVML monitoring ---
std::atomic<double> g_total_integrated_energy_joules(0.0);
std::atomic<double> g_current_power_watts(0.0);
std::atomic<bool> g_monitor_running(false);

// NVML specific globals
std::vector<nvmlDevice_t> g_nvml_devices;
std::vector<double> g_last_power_readings_W;
std::unique_ptr<std::jthread> g_monitoring_thread;
std::condition_variable g_sleep_cv;

// Data collection for CSV output
struct PowerDataPoint {
  int64_t timestamp_ms;
  double total_power_watts;
  double integrated_energy_joules;
};
std::vector<PowerDataPoint> g_power_data_points;

// Application timing
std::chrono::high_resolution_clock::time_point g_start_time;
std::chrono::high_resolution_clock::time_point g_last_measurement_time;

// --- NVML Power Monitoring Thread ---
void nvml_power_monitoring_thread_func(std::stop_token stop_token,
                                       std::chrono::milliseconds interval) {
  auto start_time           = std::chrono::high_resolution_clock::now();
  auto next_check_time      = start_time + interval;
  g_last_measurement_time   = start_time;
  int64_t interval_count    = 0;
  int64_t delayed_intervals = 0;

  while (!stop_token.stop_requested()) {
    // Interruptible sleep until next check time
    {
      std::mutex dummy_mutex;
      std::unique_lock<std::mutex> sleep_lock(dummy_mutex);
      if (g_sleep_cv.wait_for(sleep_lock, next_check_time - std::chrono::high_resolution_clock::now(), [&stop_token] {
            return stop_token.stop_requested();
          })) {
        break;  // Stop was requested during sleep
      }
    }

    auto current_time = std::chrono::high_resolution_clock::now();
    interval_count++;

    // Check if we're running behind schedule
    auto expected_time = start_time + (interval_count * interval);
    auto delay         = current_time - expected_time;

    if (delay > interval / 2) {
      delayed_intervals++;
      // Significant delay detected - adjust next check time to catch up
      next_check_time = current_time + interval;
    } else {
      // Normal case - maintain regular intervals based on expected time
      next_check_time = expected_time + interval;
    }

    double current_power_sum_W         = 0.0;
    double current_integrated_energy_J = 0.0;

    // Calculate time delta since last measurement for energy integration
    auto time_delta = std::chrono::duration_cast<std::chrono::milliseconds>(
        current_time - g_last_measurement_time);
    double time_delta_seconds = time_delta.count() / 1000.0;

    for (size_t i = 0; i < g_nvml_devices.size(); ++i) {
      if (g_nvml_devices[i] == nullptr) continue;

      // Get power usage in milliwatts
      unsigned int power_mW;
      nvmlReturn_t result =
          nvmlDeviceGetPowerUsage(g_nvml_devices[i], &power_mW);

      if (NVML_SUCCESS == result) {
        double current_power_W = static_cast<double>(power_mW) / 1000.0;
        current_power_sum_W += current_power_W;

        // Numerical integration: Energy = Power * Time
        // Use trapezoidal rule for better accuracy: E = (P_prev + P_current) /
        // 2 * dt
        if (interval_count >
            1) {  // Skip first measurement as we don't have previous power
          double avg_power_W =
              (g_last_power_readings_W[i] + current_power_W) / 2.0;
          current_integrated_energy_J += avg_power_W * time_delta_seconds;
        }

        g_last_power_readings_W[i] = current_power_W;
      }
    }

    // Update global atomic values
    g_total_integrated_energy_joules.fetch_add(current_integrated_energy_J);
    g_current_power_watts.store(current_power_sum_W);

    // Store data point for CSV output
    auto timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            current_time.time_since_epoch())
                            .count();

    g_power_data_points.push_back({timestamp_ms, current_power_sum_W,
                                   g_total_integrated_energy_joules.load()});
 
     g_last_measurement_time = current_time;
 
    // Log timing issues periodically (every 100 intervals)
    if (interval_count % 100 == 0 && delayed_intervals > 0) {
      printf("KokkosP NVML Power: Timing info - %" PRId64 " intervals, %" PRId64
             " delayed (%.1f%%)\n",
             interval_count, delayed_intervals,
             (100.0 * delayed_intervals) / interval_count);
    }
  }

  // Final timing report
  if (interval_count > 0) {
    auto total_duration =
        std::chrono::high_resolution_clock::now() - start_time;
    auto actual_avg_interval = total_duration / interval_count;
    printf(
        "KokkosP NVML Power: Monitoring completed - %" PRId64
        " intervals, avg interval: %.1f ms (expected: %" PRId64 " ms)\n",
        interval_count,
        std::chrono::duration<double, std::milli>(actual_avg_interval).count(),
        static_cast<int64_t>(interval.count()));
  }
}

// --- NVML Initialization ---
bool initialize_nvml() {
   nvmlReturn_t result = nvmlInit();
   if (NVML_SUCCESS != result) {
     std::cerr << "KokkosP NVML Power: Failed to initialize NVML: "
              << nvmlErrorString(result) << "\n";
     return false;
   }

   unsigned int device_count;
   result = nvmlDeviceGetCount(&device_count);
   if (NVML_SUCCESS != result) {
     std::cerr << "KokkosP NVML Power: Failed to get device count: "
              << nvmlErrorString(result) << "\n";
     nvmlShutdown();
     return false;
   }

   if (device_count == 0) {
     std::cerr << "KokkosP NVML Power: No NVIDIA devices found\n";
     nvmlShutdown();
     return false;
   }

   g_nvml_devices.resize(device_count);
   g_last_power_readings_W.resize(device_count, 0.0);

   printf("KokkosP NVML Power: Found %u NVIDIA device(s)\n", device_count);

   for (unsigned int i = 0; i < device_count; ++i) {
     result = nvmlDeviceGetHandleByIndex(i, &g_nvml_devices[i]);
     if (NVML_SUCCESS != result) {
       std::cerr << "KokkosP NVML Power: Failed to get handle for device " << i
                << std::endl;
       g_nvml_devices[i] = nullptr;
       continue;
     }

     // Get device name
     char device_name[NVML_DEVICE_NAME_BUFFER_SIZE];
     result = nvmlDeviceGetName(g_nvml_devices[i], device_name,
                               NVML_DEVICE_NAME_BUFFER_SIZE);
     if (NVML_SUCCESS == result) {
       printf("KokkosP NVML Power: Device %u: %s\n", i, device_name);
     }

     // Check power management mode
     nvmlEnableState_t pmmode;
     result = nvmlDeviceGetPowerManagementMode(g_nvml_devices[i], &pmmode);
     if (NVML_SUCCESS == result && pmmode == NVML_FEATURE_ENABLED) {
       printf("KokkosP NVML Power: Device %u: Power management enabled\n", i);

       // Get initial power reading
       unsigned int initial_power_mW;
      result = nvmlDeviceGetPowerUsage(g_nvml_devices[i], &initial_power_mW);
      if (NVML_SUCCESS == result) {
        double initial_power_W     = initial_power_mW / 1000.0;
        g_last_power_readings_W[i] = initial_power_W;
        printf("KokkosP NVML Power: Device %u initial power: %.3f W\n", i,
               initial_power_W);
      } else {
        printf(
            "KokkosP NVML Power: Power monitoring not supported for device "
            "%u\n",
            i);
        g_last_power_readings_W[i] = 0.0;
      }
    } else {
      printf(
          "KokkosP NVML Power: Device %u: Power management disabled or not "
          "supported\n",
          i);
      g_last_power_readings_W[i] = 0.0;
    }
  }

  return true;
}

// --- NVML Finalization ---
void finalize_nvml() {
   if (!g_nvml_devices.empty()) {
     nvmlShutdown();
   }

   g_nvml_devices.clear();
   g_last_power_readings_W.clear();
}

// --- Kokkos Profiling API Callbacks ---

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                          const uint32_t devInfoCount,
                          Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: NVML Power Profiler Initialized\n");
  printf("KokkosP: Sequence: %d, Interface Version: %llu, Devices: %u\n",
         loadSeq, (unsigned long long)interfaceVer, devInfoCount);
  printf(
      "======================================================================"
      "\n");

  g_start_time = std::chrono::high_resolution_clock::now();

  if (!initialize_nvml()) {
    printf(
        "KokkosP NVML Power: Failed to initialize NVML, power monitoring "
        "disabled\n");
    return;
  }

  // Start monitoring thread with 100ms interval using jthread
  g_monitoring_thread = std::make_unique<std::jthread>(
      nvml_power_monitoring_thread_func, std::chrono::milliseconds(100));

  printf("KokkosP NVML Power: Power monitoring started (100ms interval)\n");
}

void kokkosp_finalize_library() {
  auto end_time = std::chrono::high_resolution_clock::now();

  printf(
      "======================================================================"
      "\n");
  printf("KokkosP: NVML Power Profiler Finalization\n");

  // Stop monitoring thread - jthread handles joining automatically
  if (g_monitoring_thread) {
    g_monitoring_thread->request_stop();
    g_sleep_cv.notify_all();
    g_monitoring_thread.reset();
  }

  // Get final measurements
  double final_integrated_energy = g_total_integrated_energy_joules.load();
  double final_power             = g_current_power_watts.load();

  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
      end_time - g_start_time);
  double elapsed_seconds = duration.count() / 1000.0;

  printf("KokkosP NVML Power: Total execution time: %.3f seconds\n",
         elapsed_seconds);
  printf("KokkosP NVML Power: Total integrated energy: %.3f Joules\n",
         final_integrated_energy);
  printf("KokkosP NVML Power: Final power consumption: %.3f Watts\n",
         final_power);

  if (elapsed_seconds > 0) {
    printf("KokkosP NVML Power: Average power: %.3f Watts\n",
           final_integrated_energy / elapsed_seconds);
  }

  // Get hostname and PID for filenames
  char hostname[256];
  gethostname(hostname, 256);
  int pid = (int)getpid();

  char cwd[256];
  getcwd(cwd, 256);
 
   // Write CSV file with absolute timestamp, power and integrated energy data
   char csv_filename[512];
   snprintf(csv_filename, 512, "%s-%d-nvml-power.csv", hostname, pid);

   FILE* csv_file = fopen(csv_filename, "w");
   if (csv_file) {
     fprintf(csv_file,
             "timestamp_system_epoch_ms,nvml_power_watts,nvml_integrated_energy_"
             "joules\n");

     for (const auto& point : g_power_data_points) {
       fprintf(csv_file, "%" PRId64 ",%.6f,%.6f\n", point.timestamp_ms,
               point.total_power_watts, point.integrated_energy_joules);
     }
     fclose(csv_file);

     printf("KokkosP NVML Power: Power CSV data written to %s/%s (%" PRIu64
            " data points)\n",
            cwd, csv_filename,
            static_cast<uint64_t>(g_power_data_points.size()));
   }

   // Write relative CSV file (first measurement = time 0, power and energy from
   // baseline)
   char csv_relative_filename[512];
   snprintf(csv_relative_filename, 512, "%s-%d-nvml-power-relative.csv",
            hostname, pid);

   FILE* csv_relative_file = fopen(csv_relative_filename, "w");
   if (csv_relative_file && !g_power_data_points.empty()) {
     fprintf(csv_relative_file,
             "time_relative_ms,power_watts,energy_relative_joules\n");

     // Use first data point as reference (time=0, energy=0, but keep absolute
     // power values)
     int64_t first_timestamp = g_power_data_points[0].timestamp_ms;
     double first_energy     = g_power_data_points[0].integrated_energy_joules;

     for (const auto& point : g_power_data_points) {
       int64_t relative_time_ms = point.timestamp_ms - first_timestamp;
       double relative_energy_joules =
           point.integrated_energy_joules - first_energy;
       fprintf(csv_relative_file, "%" PRId64 ",%.6f,%.6f\n", relative_time_ms,
               point.total_power_watts, relative_energy_joules);
     }
     fclose(csv_relative_file);

     printf("KokkosP NVML Power: Relative power CSV data written to %s/%s\n",
            cwd, csv_relative_filename);
   }

   // Write DAT file with calculated statistics
   char dat_filename[512];
   snprintf(dat_filename, 512, "%s-%d-nvml-power.dat", hostname, pid);

   FILE* dat_file = fopen(dat_filename, "w");
   if (dat_file) {
     fprintf(dat_file, "# NVML Power Profiler Results\n");
     fprintf(dat_file, "execution_time_seconds: %.6f\n", elapsed_seconds);
     fprintf(dat_file, "total_integrated_energy_joules: %.6f\n",
             final_integrated_energy);
     fprintf(dat_file, "final_power_watts: %.6f\n", final_power);
     if (elapsed_seconds > 0) {
       fprintf(dat_file, "average_power_watts: %.6f\n",
               final_integrated_energy / elapsed_seconds);
     }

     // Calculate additional statistics from data points
     if (!g_power_data_points.empty()) {
       double min_power = std::numeric_limits<double>::max();
       double max_power = std::numeric_limits<double>::lowest();
       double sum_power = 0.0;

       for (const auto& point : g_power_data_points) {
         min_power = std::min(min_power, point.total_power_watts);
         max_power = std::max(max_power, point.total_power_watts);
         sum_power += point.total_power_watts;
       }

       double avg_power = sum_power / g_power_data_points.size();

       fprintf(dat_file, "min_power_watts: %.6f\n", min_power);
       fprintf(dat_file, "max_power_watts: %.6f\n", max_power);
       fprintf(dat_file, "average_measured_power_watts: %.6f\n", avg_power);
       fprintf(dat_file, "num_power_measurements: %" PRIu64 "\n",
               static_cast<uint64_t>(g_power_data_points.size()));
     }

     fclose(dat_file);

     printf("KokkosP NVML Power: Power summary written to %s/%s\n", cwd,
            dat_filename);
   }

  finalize_nvml();
  printf(
      "======================================================================"
      "\n");
}

// --- Kernel callbacks (simplified for power profiling) ---

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                                uint64_t* kID) {
}

void kokkosp_end_parallel_for(const uint64_t kID) {
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
}

void kokkosp_end_parallel_scan(const uint64_t kID) {}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                   uint64_t* kID) {
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {}

void kokkosp_push_profile_region(char const* regionName) {}

void kokkosp_pop_profile_region() {}

// --- Event Set Configuration ---
Kokkos::Tools::Experimental::EventSet get_event_set() {
  Kokkos::Tools::Experimental::EventSet my_event_set;
  memset(&my_event_set, 0, sizeof(my_event_set));
  my_event_set.init                  = kokkosp_init_library;
  my_event_set.finalize              = kokkosp_finalize_library;
  my_event_set.begin_parallel_for    = kokkosp_begin_parallel_for;
  my_event_set.begin_parallel_reduce = kokkosp_begin_parallel_reduce;
  my_event_set.begin_parallel_scan   = kokkosp_begin_parallel_scan;
  my_event_set.end_parallel_for      = kokkosp_end_parallel_for;
  my_event_set.end_parallel_reduce   = kokkosp_end_parallel_reduce;
  my_event_set.end_parallel_scan     = kokkosp_end_parallel_scan;
  my_event_set.push_region           = kokkosp_push_profile_region;
  my_event_set.pop_region            = kokkosp_pop_profile_region;
  return my_event_set;
}

}  // namespace NVMLPowerProfiler
}  // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::NVMLPowerProfiler;

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

}  // extern "C"
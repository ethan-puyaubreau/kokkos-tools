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
#include <ostream>
#include <string>

namespace KokkosTools {
namespace PowerProfiler {

struct ProfilerConfig {
  std::chrono::microseconds monitor_interval{20000};  // 20ms in microseconds
  size_t initial_data_capacity{
      10000};  // Pre-allocate for ~3min at 20ms intervals

  // Provider configuration
  enum class ProviderType {
    VARIORUM,
    DUMMY,  // For testing
    PAPI    // Future: PAPI support
  };
  ProviderType energy_provider_type{ProviderType::VARIORUM};

  // Output configuration
  enum class OutputType { CONSOLE, JSON_FILE, CSV_FILE };
  OutputType output_type{OutputType::CONSOLE};
  std::string output_file_path{"power_profile_output"};

  bool fail_on_provider_error{
      false};  //! Only for debugging, will stop profiling if the provider fails
  bool enable_thread_safety{true};

  using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
  using Duration  = std::chrono::milliseconds;

  static std::string provider_type_to_string(ProviderType type) {
    switch (type) {
      case ProviderType::VARIORUM: return "VARIORUM";
      case ProviderType::DUMMY: return "DUMMY";
      case ProviderType::PAPI: return "PAPI";
      default: return "UNKNOWN";
    }
  }

  static std::string output_type_to_string(OutputType type) {
    switch (type) {
      case OutputType::CONSOLE: return "CONSOLE";
      case OutputType::JSON_FILE: return "JSON_FILE";
      case OutputType::CSV_FILE: return "CSV_FILE";
      default: return "UNKNOWN";
    }
  }

  friend std::ostream& operator<<(std::ostream& os,
                                  const ProfilerConfig& config) {
    os << "ProfilerConfig:\n"
       << "  Monitor Interval: " << config.monitor_interval.count()
       << " microseconds\n"
       << "  Initial Data Capacity: " << config.initial_data_capacity << "\n"
       << "  Energy Provider Type: "
       << provider_type_to_string(config.energy_provider_type) << "\n"
       << "  Output Type: " << output_type_to_string(config.output_type) << "\n"
       << "  Output File Path: " << config.output_file_path << "\n"
       << "  Fail on Provider Error: "
       << (config.fail_on_provider_error ? "true" : "false") << "\n"
       << "  Enable Thread Safety: "
       << (config.enable_thread_safety ? "true" : "false") << "\n";
    return os;
  }

  void load_from_environment();
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

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

#include "config.hpp"
#include <cstdlib>
#include <iostream>

namespace KokkosTools {
namespace PowerProfiler {

void ProfilerConfig::load_from_environment() {
  // Monitor interval
  if (const char* interval_str =
          std::getenv("KOKKOS_ENERGY_MONITOR_INTERVAL_MS")) {
    try {
      int interval_ms = std::stoi(interval_str);
      if (interval_ms > 0) {
        monitor_interval = std::chrono::microseconds(interval_ms * 1000);
      }
    } catch (const std::exception& e) {
      std::cerr << "Warning: Invalid KOKKOS_ENERGY_MONITOR_INTERVAL_MS value: "
                << interval_str << "\n";
    }
  }

  // Provider type
  if (const char* provider_str = std::getenv("KOKKOS_ENERGY_ENERGY_PROVIDER")) {
    std::string provider(provider_str);
    if (provider == "VARIORUM") {
      energy_provider_type = ProviderType::VARIORUM;
    } else if (provider == "DUMMY") {
      energy_provider_type = ProviderType::DUMMY;
    } else if (provider == "PAPI") {
      energy_provider_type = ProviderType::PAPI;
    } else {
      std::cerr << "Warning: Unknown energy provider: " << provider << "\n";
    }
  }

  // Output type
  if (const char* output_str = std::getenv("KOKKOS_ENERGY_OUTPUT_TYPE")) {
    std::string output(output_str);
    if (output == "CONSOLE") {
      output_type = OutputType::CONSOLE;
    } else if (output == "JSON") {
      output_type = OutputType::JSON_FILE;
    } else if (output == "CSV") {
      output_type = OutputType::CSV_FILE;
    } else {
      std::cerr << "Warning: Unknown output type: " << output << "\n";
    }
  }

  // Output file path
  if (const char* path_str = std::getenv("KOKKOS_ENERGY_OUTPUT_PATH")) {
    output_file_path = std::string(path_str);
  }

  // Error handling
  if (const char* fail_str = std::getenv("KOKKOS_ENERGY_FAIL_ON_ERROR")) {
    fail_on_provider_error =
        (std::string(fail_str) == "1" || std::string(fail_str) == "true");
  }
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

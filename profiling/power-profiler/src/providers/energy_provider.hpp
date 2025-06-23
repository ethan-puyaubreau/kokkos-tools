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

#include "../data/energy_metrics.hpp"
#include <vector>
#include <memory>

namespace KokkosTools {
namespace PowerProfiler {

class EnergyProvider {
 public:
  virtual ~EnergyProvider() = default;

  virtual void initialize()                                   = 0;
  virtual std::vector<uint32_t> get_available_devices() const = 0;
  virtual EnergyReading get_current_reading() const           = 0;
  virtual void finalize()                                     = 0;
};

class EnergyProviderFactory {
 public:
  enum class ProviderType {
    VARIORUM,
    DUMMY,
    PAPI,      // Future: Intel PAPI support
    NVIDIA_ML  // Future: NVIDIA Management Library support
  };

  static std::unique_ptr<EnergyProvider> create(ProviderType type);

  // Auto-detect best available provider
  static std::unique_ptr<EnergyProvider> create_best_available();

  // Get provider type from string (for environment variable parsing)
  static ProviderType string_to_provider_type(const std::string& type_str);
  static std::string provider_type_to_string(ProviderType type);
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

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

#include "energy_provider.hpp"
#include "variorum_energy_provider.hpp"
#include "dummy_energy_provider.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>

namespace KokkosTools {
namespace PowerProfiler {

std::unique_ptr<EnergyProvider> EnergyProviderFactory::create(
    ProviderType type) {
  switch (type) {
    case ProviderType::VARIORUM:
      try {
        return std::make_unique<VariorumEnergyProvider>();
      } catch (const std::exception& e) {
        std::cerr << "Failed to create Variorum provider: " << e.what() << "\n";
        throw;
      }
    case ProviderType::DUMMY: return std::make_unique<DummyEnergyProvider>();
    case ProviderType::PAPI:
      throw std::runtime_error("PAPI provider not yet implemented");
    case ProviderType::NVIDIA_ML:
      throw std::runtime_error("NVIDIA ML provider not yet implemented");
    default: throw std::runtime_error("Unknown energy provider type");
  }
}

std::unique_ptr<EnergyProvider> EnergyProviderFactory::create_best_available() {
  // Try providers in order of preference
  std::vector<ProviderType> providers_to_try = {
      ProviderType::VARIORUM,
      // ProviderType::PAPI,     // When implemented
      // ProviderType::NVIDIA_ML, // When implemented
      ProviderType::DUMMY  // Fallback
  };

  for (auto type : providers_to_try) {
    try {
      auto provider = create(type);
      std::cout << "Successfully created energy provider: "
                << provider_type_to_string(type) << "\n";
      return provider;
    } catch (const std::exception& e) {
      std::cout << "Failed to create " << provider_type_to_string(type)
                << " provider: " << e.what() << "\n";
      continue;
    }
  }

  throw std::runtime_error("No energy provider could be created");
}

EnergyProviderFactory::ProviderType
EnergyProviderFactory::string_to_provider_type(const std::string& type_str) {
  if (type_str == "VARIORUM") return ProviderType::VARIORUM;
  if (type_str == "DUMMY") return ProviderType::DUMMY;
  if (type_str == "PAPI") return ProviderType::PAPI;
  if (type_str == "NVIDIA_ML") return ProviderType::NVIDIA_ML;

  throw std::runtime_error("Unknown energy provider type: " + type_str);
}

std::string EnergyProviderFactory::provider_type_to_string(ProviderType type) {
  switch (type) {
    case ProviderType::VARIORUM: return "VARIORUM";
    case ProviderType::DUMMY: return "DUMMY";
    case ProviderType::PAPI: return "PAPI";
    case ProviderType::NVIDIA_ML: return "NVIDIA_ML";
    default: return "UNKNOWN";
  }
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

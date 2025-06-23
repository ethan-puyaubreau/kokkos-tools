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

#include "energy_provider.hpp"
#include <random>

namespace KokkosTools {
namespace PowerProfiler {

// Dummy provider for testing and fallback when Variorum is not available
class DummyEnergyProvider : public EnergyProvider {
 public:
  DummyEnergyProvider();

  void initialize() override;
  std::vector<uint32_t> get_available_devices() const override;
  EnergyReading get_current_reading() const override;
  void finalize() override;

 private:
  mutable std::mt19937 rng_;
  mutable std::uniform_real_distribution<double> power_dist_;
  std::vector<uint32_t> dummy_devices_;
  bool initialized_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

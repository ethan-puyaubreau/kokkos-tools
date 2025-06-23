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

#include "dummy_energy_provider.hpp"
#include <iostream>
#include <chrono>

namespace KokkosTools {
namespace PowerProfiler {

DummyEnergyProvider::DummyEnergyProvider()
    : rng_(std::chrono::steady_clock::now().time_since_epoch().count()),
      power_dist_(50.0, 150.0)  // Simulated power range 50-150W
      ,
      initialized_(false) {
  // Simulate 2 GPUs
  dummy_devices_ = {0, 1};
}

void DummyEnergyProvider::initialize() {
  std::cout << "DummyEnergyProvider: Initializing dummy energy monitoring"
            << "\n";
  initialized_ = true;
}

std::vector<uint32_t> DummyEnergyProvider::get_available_devices() const {
  return dummy_devices_;
}

EnergyReading DummyEnergyProvider::get_current_reading() const {
  if (!initialized_) {
    throw std::runtime_error("DummyEnergyProvider not initialized");
  }

  auto timestamp = std::chrono::steady_clock::now();
  std::map<uint32_t, double> device_powers;

  // Generate dummy power readings with some variance
  for (uint32_t device_id : dummy_devices_) {
    double base_power =
        80.0 + (device_id * 20.0);  // Device 0: ~80W, Device 1: ~100W
    double variance = power_dist_(rng_) - 100.0;  // ±50W variance
    device_powers[device_id] =
        base_power + (variance * 0.1);  // ±5W actual variance
  }

  return EnergyReading(timestamp, std::move(device_powers));
}

void DummyEnergyProvider::finalize() {
  std::cout << "DummyEnergyProvider: Finalizing dummy energy monitoring"
            << "\n";
  initialized_ = false;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

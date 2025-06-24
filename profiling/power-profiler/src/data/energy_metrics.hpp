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

#include <string>
#include <chrono>
#include <cstdint>
#include <vector>

namespace KokkosTools {
namespace PowerProfiler {

// Device power reading with both ID and name
struct DevicePowerReading {
  uint32_t device_id;
  std::string device_name;
  double power_watts;

  DevicePowerReading() = default;
  DevicePowerReading(uint32_t id, std::string name, double power)
      : device_id(id), device_name(std::move(name)), power_watts(power) {}
};

struct EnergyReading {
  std::chrono::time_point<std::chrono::system_clock> timestamp;
  std::vector<DevicePowerReading> device_readings;
  
  EnergyReading() = default;
  EnergyReading(std::chrono::time_point<std::chrono::system_clock> ts,
                std::vector<DevicePowerReading> readings)
      : timestamp(ts), device_readings(std::move(readings)) {}
  
  // Get power for a specific device ID
  double get_power_for_device(uint32_t device_id) const;
  
  // Get total power across all devices
  double get_total_power() const;

  int64_t timestamp_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               timestamp.time_since_epoch())
        .count();
  }
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

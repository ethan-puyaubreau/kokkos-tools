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

#include <map>
#include <chrono>
#include <cstdint>

namespace KokkosTools {
namespace PowerProfiler {

struct EnergyReading {
  std::chrono::time_point<std::chrono::system_clock> timestamp;
  std::map<uint32_t, double> device_powers;

  EnergyReading() = default;
  EnergyReading(std::chrono::time_point<std::chrono::system_clock> ts,
                std::map<uint32_t, double> powers)
      : timestamp(ts), device_powers(std::move(powers)) {}

  // Primary millisecond-based timestamp (preferred)
  int64_t timestamp_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               timestamp.time_since_epoch())
        .count();
  }

  // Legacy nanosecond timestamp (for backward compatibility if needed)
  int64_t timestamp_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               timestamp.time_since_epoch())
        .count();
  }
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

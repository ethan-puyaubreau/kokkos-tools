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

namespace KokkosTools {
namespace PowerProfiler {

enum class KernelType { FOR, SCAN, REDUCE, UNKNOWN };

struct KernelTiming {
  std::string name;
  KernelType type;
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;

  KernelTiming() = default;
  KernelTiming(std::string n, KernelType t,
               std::chrono::time_point<std::chrono::system_clock> start)
      : name(std::move(n)), type(t), start_time(start), end_time() {}

  std::chrono::nanoseconds duration() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                                start_time);
  }

  std::chrono::milliseconds duration_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                 start_time);
  }

  // Primary millisecond-based methods (preferred)
  int64_t start_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               start_time.time_since_epoch())
        .count();
  }

  int64_t end_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               end_time.time_since_epoch())
        .count();
  }

  int64_t duration_ms_value() const { return duration_ms().count(); }

  // Legacy nanosecond methods (for backward compatibility if needed)
  int64_t start_time_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               start_time.time_since_epoch())
        .count();
  }

  int64_t end_time_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               end_time.time_since_epoch())
        .count();
  }

  int64_t duration_ns() const { return duration().count(); }
};

struct RegionTiming {
  std::string name;
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;

  RegionTiming() = default;
  RegionTiming(std::string n,
               std::chrono::time_point<std::chrono::system_clock> start)
      : name(std::move(n)), start_time(start), end_time() {}

  std::chrono::nanoseconds duration() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time -
                                                                start_time);
  }

  std::chrono::milliseconds duration_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time -
                                                                 start_time);
  }

  // Primary millisecond-based methods (preferred)
  int64_t start_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               start_time.time_since_epoch())
        .count();
  }

  int64_t end_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               end_time.time_since_epoch())
        .count();
  }

  int64_t duration_ms_value() const { return duration_ms().count(); }

  // Legacy nanosecond methods (for backward compatibility if needed)
  int64_t start_time_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               start_time.time_since_epoch())
        .count();
  }

  int64_t end_time_ns() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               end_time.time_since_epoch())
        .count();
  }

  int64_t duration_ns() const { return duration().count(); }
};

std::string kernel_type_to_string(KernelType type);

}  // namespace PowerProfiler
}  // namespace KokkosTools

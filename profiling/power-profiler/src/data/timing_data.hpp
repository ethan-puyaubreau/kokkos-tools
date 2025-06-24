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
#include <atomic>
#include <cstdint>

namespace KokkosTools {
namespace PowerProfiler {

// Global ID generators for kernels and regions
extern std::atomic<uint64_t> global_kernel_id;
extern std::atomic<uint64_t> global_region_id;

enum class KernelType { FOR, SCAN, REDUCE, UNKNOWN };

enum class ExecutionSpace {
  HOST_SERIAL,
  HOST_OPENMP,
  HOST_THREADS,
  DEVICE_CUDA,
  DEVICE_HIP,
  DEVICE_SYCL,
  DEVICE_OPENMPTARGET,
  DEVICE_OPENACC,
  UNKNOWN
};

struct KernelTiming {
  uint64_t id;                 // Unique ID for this kernel
  std::string name;
  KernelType type;
  ExecutionSpace execution_space;
  uint32_t device_id;
  uint32_t instance_id;
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;

  KernelTiming() : id(global_kernel_id++) {}
  KernelTiming(std::string n, KernelType t, ExecutionSpace space, 
               uint32_t dev_id, uint32_t inst_id,
               std::chrono::time_point<std::chrono::system_clock> start)
      : id(global_kernel_id++), name(std::move(n)), type(t), execution_space(space), 
        device_id(dev_id), instance_id(inst_id), start_time(start), end_time() {}

  std::chrono::nanoseconds duration() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
  }

  std::chrono::milliseconds duration_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  }

  // Primary millisecond-based methods (preferred)
  int64_t start_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               start_time.time_since_epoch()).count();
  }

  int64_t end_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               end_time.time_since_epoch()).count();
  }

  int64_t duration_ms_value() const { return duration_ms().count(); }
};

struct RegionTiming {
  uint64_t id;                 // Unique ID for this region
  std::string name;
  std::chrono::time_point<std::chrono::system_clock> start_time;
  std::chrono::time_point<std::chrono::system_clock> end_time;

  RegionTiming() : id(global_region_id++) {}
  RegionTiming(std::string n,
               std::chrono::time_point<std::chrono::system_clock> start)
      : id(global_region_id++), name(std::move(n)), start_time(start), end_time() {}

  std::chrono::nanoseconds duration() const {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
  }

  std::chrono::milliseconds duration_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
  }

  // Primary millisecond-based methods (preferred)
  int64_t start_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               start_time.time_since_epoch()).count();
  }

  int64_t end_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               end_time.time_since_epoch()).count();
  }

  int64_t duration_ms_value() const { return duration_ms().count(); }
};

std::string kernel_type_to_string(KernelType type);
std::string execution_space_to_string(ExecutionSpace space);
bool is_host_execution_space(ExecutionSpace space);
bool is_device_execution_space(ExecutionSpace space);

}  // namespace PowerProfiler
}  // namespace KokkosTools

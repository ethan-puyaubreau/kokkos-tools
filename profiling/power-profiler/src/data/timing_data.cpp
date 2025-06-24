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

#include "timing_data.hpp"

namespace KokkosTools {
namespace PowerProfiler {

// Initialize global ID counters
std::atomic<uint64_t> global_kernel_id{0};
std::atomic<uint64_t> global_region_id{0};

std::string kernel_type_to_string(KernelType type) {
  switch (type) {
    case KernelType::FOR: return "For";
    case KernelType::SCAN: return "Scan";
    case KernelType::REDUCE: return "Reduce";
    case KernelType::UNKNOWN: return "Unknown";
  }
  return "Unknown";
}

std::string execution_space_to_string(ExecutionSpace space) {
  switch (space) {
    case ExecutionSpace::HOST_SERIAL: return "Serial";
    case ExecutionSpace::HOST_OPENMP: return "OpenMP";
    case ExecutionSpace::HOST_THREADS: return "Threads";
    case ExecutionSpace::DEVICE_CUDA: return "Cuda";
    case ExecutionSpace::DEVICE_HIP: return "HIP";
    case ExecutionSpace::DEVICE_SYCL: return "SYCL";
    case ExecutionSpace::DEVICE_OPENMPTARGET: return "OpenMPTarget";
    case ExecutionSpace::DEVICE_OPENACC: return "OpenACC";
    case ExecutionSpace::UNKNOWN: return "Unknown";
  }
  return "Unknown";
}

bool is_host_execution_space(ExecutionSpace space) {
  return space == ExecutionSpace::HOST_SERIAL ||
         space == ExecutionSpace::HOST_OPENMP ||
         space == ExecutionSpace::HOST_THREADS;
}

bool is_device_execution_space(ExecutionSpace space) {
  return space == ExecutionSpace::DEVICE_CUDA ||
         space == ExecutionSpace::DEVICE_HIP ||
         space == ExecutionSpace::DEVICE_SYCL ||
         space == ExecutionSpace::DEVICE_OPENMPTARGET ||
         space == ExecutionSpace::DEVICE_OPENACC;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

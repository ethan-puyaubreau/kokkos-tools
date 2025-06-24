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

#include "device_utils.hpp"

namespace KokkosTools {
namespace PowerProfiler {

ExecutionSpace device_type_to_execution_space(
    Kokkos::Tools::Experimental::DeviceType device_type) {
  switch (device_type) {
    case Kokkos::Tools::Experimental::DeviceType::Serial:
      return ExecutionSpace::HOST_SERIAL;
    case Kokkos::Tools::Experimental::DeviceType::OpenMP:
      return ExecutionSpace::HOST_OPENMP;
    case Kokkos::Tools::Experimental::DeviceType::Threads:
      return ExecutionSpace::HOST_THREADS;
    case Kokkos::Tools::Experimental::DeviceType::Cuda:
      return ExecutionSpace::DEVICE_CUDA;
    case Kokkos::Tools::Experimental::DeviceType::HIP:
      return ExecutionSpace::DEVICE_HIP;
    case Kokkos::Tools::Experimental::DeviceType::SYCL:
      return ExecutionSpace::DEVICE_SYCL;
    case Kokkos::Tools::Experimental::DeviceType::OpenMPTarget:
      return ExecutionSpace::DEVICE_OPENMPTARGET;
    case Kokkos::Tools::Experimental::DeviceType::OpenACC:
      return ExecutionSpace::DEVICE_OPENACC;
    case Kokkos::Tools::Experimental::DeviceType::Unknown:
    default:
      return ExecutionSpace::UNKNOWN;
  }
}

Kokkos::Tools::Experimental::ExecutionSpaceIdentifier 
get_execution_space_info(uint32_t device_id) {
  return Kokkos::Tools::Experimental::identifier_from_devid(device_id);
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

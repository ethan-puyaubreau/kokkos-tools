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

std::string kernel_type_to_string(KernelType type) {
  switch (type) {
    case KernelType::FOR: return "For";
    case KernelType::SCAN: return "Scan";
    case KernelType::REDUCE: return "Reduce";
    case KernelType::UNKNOWN: return "Unknown";
  }
  return "Unknown";
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

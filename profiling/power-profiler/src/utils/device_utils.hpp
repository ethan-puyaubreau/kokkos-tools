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

#include "../data/timing_data.hpp"
#include "impl/Kokkos_Profiling_Interface.hpp"

namespace KokkosTools {
namespace PowerProfiler {

/**
 * Convert Kokkos DeviceType to our ExecutionSpace enum
 */
ExecutionSpace device_type_to_execution_space(
    Kokkos::Tools::Experimental::DeviceType device_type);

/**
 * Get ExecutionSpaceIdentifier from device ID
 */
Kokkos::Tools::Experimental::ExecutionSpaceIdentifier 
get_execution_space_info(uint32_t device_id);

}  // namespace PowerProfiler
}  // namespace KokkosTools

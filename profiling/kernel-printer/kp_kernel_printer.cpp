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

#include <iostream>
#include <fstream>
#include "kp_core.hpp"

namespace KokkosTools {
namespace KernelPrinter {

static std::ofstream kernel_info_file;

void kokkosp_init_library(const int loadSeq, const uint64_t interfaceVer,
                         const uint32_t devInfoCount,
                         Kokkos_Profiling_KokkosPDeviceInfo* deviceInfo) {
  kernel_info_file.open("kokkos_kernels.info");
  kernel_info_file << "KokkosP: Simple Kernel Printer Tool Initialized (sequence is " 
            << loadSeq << ", version: " << interfaceVer << ")" << std::endl;
}

void kokkosp_finalize_library() {
  kernel_info_file << "KokkosP: Simple Kernel Printer Tool Finalized" << std::endl;
  kernel_info_file.close();
}

void kokkosp_begin_parallel_for(const char* name, const uint32_t devID,
                              uint64_t* kID) {
  kernel_info_file << "KERNEL_TYPE: parallel_for, NAME: " << name << std::endl;
}

void kokkosp_end_parallel_for(const uint64_t kID) {
  kernel_info_file << "END_KERNEL: parallel_for" << std::endl;
}

void kokkosp_begin_parallel_scan(const char* name, const uint32_t devID,
                               uint64_t* kID) {
  kernel_info_file << "KERNEL_TYPE: parallel_scan, NAME: " << name << std::endl;
}

void kokkosp_end_parallel_scan(const uint64_t kID) {
  kernel_info_file << "END_KERNEL: parallel_scan" << std::endl;
}

void kokkosp_begin_parallel_reduce(const char* name, const uint32_t devID,
                                 uint64_t* kID) {
  kernel_info_file << "KERNEL_TYPE: parallel_reduce, NAME: " << name << std::endl;
}

void kokkosp_end_parallel_reduce(const uint64_t kID) {
  kernel_info_file << "END_KERNEL: parallel_reduce" << std::endl;
}

void kokkosp_push_profile_region(char const* regionName) {
  kernel_info_file << "KokkosP: Entering region: " << regionName << std::endl;
}

void kokkosp_pop_profile_region() {
  kernel_info_file << "KokkosP: Exiting region" << std::endl;
}

} // namespace KernelPrinter
} // namespace KokkosTools

extern "C" {

namespace impl = KokkosTools::KernelPrinter;

EXPOSE_INIT(impl::kokkosp_init_library)
EXPOSE_FINALIZE(impl::kokkosp_finalize_library)
EXPOSE_BEGIN_PARALLEL_FOR(impl::kokkosp_begin_parallel_for)
EXPOSE_END_PARALLEL_FOR(impl::kokkosp_end_parallel_for)
EXPOSE_BEGIN_PARALLEL_SCAN(impl::kokkosp_begin_parallel_scan)
EXPOSE_END_PARALLEL_SCAN(impl::kokkosp_end_parallel_scan)
EXPOSE_BEGIN_PARALLEL_REDUCE(impl::kokkosp_begin_parallel_reduce)
EXPOSE_END_PARALLEL_REDUCE(impl::kokkosp_end_parallel_reduce)
EXPOSE_PUSH_REGION(impl::kokkosp_push_profile_region)
EXPOSE_POP_REGION(impl::kokkosp_pop_profile_region)

} // extern "C"

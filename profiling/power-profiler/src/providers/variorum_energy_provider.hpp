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

#include "energy_provider.hpp"
#include <memory>

extern "C" {
#include <variorum.h>
#include <jansson.h>
}

namespace KokkosTools {
namespace PowerProfiler {

class VariorumEnergyProvider : public EnergyProvider {
 public:
  void initialize() override;
  std::vector<uint32_t> get_available_devices() const override;
  EnergyReading get_current_reading() const override;
  void finalize() override;

 private:
  struct JsonDeleter {
    void operator()(json_t* json) const {
      if (json) {
        json_decref(json);
      }
    }
  };
  using unique_json_ptr = std::unique_ptr<json_t, JsonDeleter>;

  struct CFreeDeleter {
    void operator()(char* ptr) const {
      if (ptr) {
        free(ptr);
      }
    }
  };
  using unique_c_string_ptr = std::unique_ptr<char, CFreeDeleter>;

  unique_json_ptr get_variorum_json_data() const;
  std::map<uint32_t, double> get_current_power_for_devices(
      const std::vector<uint32_t>& device_ids) const;

  std::vector<uint32_t> available_devices_;
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

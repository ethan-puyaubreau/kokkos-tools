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

#include "variorum_energy_provider.hpp"
#include <iostream>
#include <set>
#include <string>
#include <stdexcept>

namespace KokkosTools {
namespace PowerProfiler {

void VariorumEnergyProvider::initialize() {
  available_devices_ = get_available_devices();
}

std::vector<uint32_t> VariorumEnergyProvider::get_available_devices() const {
  std::set<uint32_t> found_device_ids;
  unique_json_ptr root = get_variorum_json_data();

  if (!root) {
    return {};
  }

  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    std::cerr << "PowerProfiler: No hostname object found in JSON." 
    << "\n";
    return {};
  }

  json_t* socket_0 = json_object_get(host_obj, "socket_0");
  if (!socket_0 || !json_is_object(socket_0)) {
    std::cerr << "PowerProfiler: 'socket_0' object not found or invalid."
              << "\n";
    return {};
  }

  json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
  if (!power_gpu_watts || !json_is_object(power_gpu_watts)) {
    std::cerr << "PowerProfiler: 'power_gpu_watts' object not found or invalid."
              << "\n";
    return {};
  }

  const char* key;
  json_t* value;
  json_object_foreach(power_gpu_watts, key, value) {
    std::string s_key(key);
    if (s_key.length() > 4 && s_key.substr(0, 4) == "GPU_") {
      try {
        uint32_t device_id = std::stoul(s_key.substr(4));
        found_device_ids.insert(device_id);
      } catch (const std::invalid_argument& e) {
        std::cerr << "PowerProfiler: Could not parse GPU ID from key: " << s_key
                  << " (" << e.what() << ")" 
                  << "\n";
      } catch (const std::out_of_range& e) {
        std::cerr << "PowerProfiler: GPU ID out of range from key: " << s_key
                  << " (" << e.what() << ")" 
                  << "\n";
      }
    }
  }

  return std::vector<uint32_t>(found_device_ids.begin(),
                               found_device_ids.end());
}

EnergyReading VariorumEnergyProvider::get_current_reading() const {
  auto current_time = std::chrono::system_clock::now();

  std::map<uint32_t, double> power_readings =
      get_current_power_for_devices(available_devices_);

  return EnergyReading(current_time, std::move(power_readings));
}

void VariorumEnergyProvider::finalize() {}

VariorumEnergyProvider::unique_json_ptr
VariorumEnergyProvider::get_variorum_json_data() const {
  char* json_string_c_raw = nullptr;
  int variorum_error      = variorum_get_power_json(&json_string_c_raw);

  if (variorum_error != 0) {
    std::cerr << "PowerProfiler: variorum_get_power_json() failed. Error code: "
              << variorum_error << "\n";
    return nullptr;
  }

  unique_c_string_ptr json_string_c(json_string_c_raw);

  if (!json_string_c) {
    std::cerr << "PowerProfiler: variorum_get_power_json() returned success "
                 "but a null pointer."
              << "\n";
    return nullptr;
  }

  json_error_t error;
  json_t* root_ptr = json_loads(json_string_c.get(), 0, &error);

  if (!root_ptr) {
    std::cerr << "PowerProfiler: Failed to parse JSON: " << error.text << "\n";
    return nullptr;
  }

  return unique_json_ptr(root_ptr);
}

std::map<uint32_t, double>
VariorumEnergyProvider::get_current_power_for_devices(
    const std::vector<uint32_t>& device_ids) const {
  std::map<uint32_t, double> power_readings;
  unique_json_ptr root = get_variorum_json_data();

  if (!root) {
    return {};
  }

  json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
  if (!host_obj) {
    return {};
  }

  json_t* socket_0 = json_object_get(host_obj, "socket_0");
  if (!socket_0 || !json_is_object(socket_0)) {
    return {};
  }

  json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
  if (!power_gpu_watts || !json_is_object(power_gpu_watts)) {
    return {};
  }

  for (uint32_t device_id : device_ids) {
    std::string gpu_key = "GPU_" + std::to_string(device_id);
    json_t* power_value = json_object_get(power_gpu_watts, gpu_key.c_str());

    if (json_is_number(power_value)) {
      power_readings[device_id] = json_number_value(power_value);
    }
  }

  return power_readings;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

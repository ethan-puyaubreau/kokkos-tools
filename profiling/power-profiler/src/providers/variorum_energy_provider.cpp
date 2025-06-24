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
  
  // Initialize device name map
  unique_json_ptr root = get_variorum_json_data();
  if (root) {
    json_t* host_obj = json_object_iter_value(json_object_iter(root.get()));
    if (host_obj) {
      json_t* socket_0 = json_object_get(host_obj, "socket_0");
      if (socket_0 && json_is_object(socket_0)) {
        json_t* power_gpu_watts = json_object_get(socket_0, "power_gpu_watts");
        if (power_gpu_watts && json_is_object(power_gpu_watts)) {
          const char* key;
          json_t* value;
          json_object_foreach(power_gpu_watts, key, value) {
            std::string s_key(key);
            if (s_key.length() > 4 && s_key.substr(0, 4) == "GPU_") {
              try {
                uint32_t device_id = std::stoul(s_key.substr(4));
                device_names_[device_id] = s_key;
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
        }
      }
    }
  }
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
  
  // Get power readings for all available devices
  std::map<uint32_t, double> power_map = 
      get_current_power_for_devices(available_devices_);
      
  // Convert to our new format
  std::vector<DevicePowerReading> device_readings;
  device_readings.reserve(power_map.size());
  
  for (const auto& [device_id, power] : power_map) {
    std::string device_name;
    // Look up device name
    auto it = device_names_.find(device_id);
    if (it != device_names_.end()) {
      device_name = it->second;
    } else {
      device_name = "GPU_" + std::to_string(device_id);
    }
    device_readings.emplace_back(device_id, device_name, power);
  }

  return EnergyReading(current_time, std::move(device_readings));
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

  if (!json_string_c_raw) {
    std::cerr << "PowerProfiler: variorum_get_power_json() returned null."
              << "\n";
    return nullptr;
  }

  json_error_t error;
  json_t* root = json_loads(json_string_c_raw, 0, &error);
  free(json_string_c_raw);

  if (!root) {
    std::cerr << "PowerProfiler: Error parsing JSON: " << error.text
              << " at line " << error.line << "\n";
    return nullptr;
  }

  return unique_json_ptr(root);
}

std::map<uint32_t, double> VariorumEnergyProvider::get_current_power_for_devices(
    const std::vector<uint32_t>& devices) const {
  std::map<uint32_t, double> result;
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

  for (uint32_t device_id : devices) {
    std::string key = "GPU_" + std::to_string(device_id);
    json_t* value   = json_object_get(power_gpu_watts, key.c_str());

    if (!value || !json_is_number(value)) {
      std::cerr << "PowerProfiler: Power data for " << key
                << " not found or invalid."
                << "\n";
      continue;
    }

    double power = json_number_value(value);
    result[device_id] = power;
  }

  return result;
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

#pragma once

#include <vector>
#include <string>
#include <nvml.h>
#include "../common/error_handling.hpp"

namespace KokkosTools {
namespace EnergyProfiler {

/**
 * @brief NVML Power Provider with improved error handling
 *
 * Provides simplified power monitoring using NVML APIs with
 * consistent error reporting and status checking.
 */
class NVMLProvider {
 public:
  NVMLProvider();
  ~NVMLProvider();

  // Initialize NVML and discover devices
  Result initialize();

  // Cleanup NVML resources
  void finalize();

  // Get current power consumption in Watts for all devices
  Result get_total_power_usage(double& power_watts);

  // Get power usage for a specific device
  Result get_device_power_usage(size_t device_index, double& power_watts);

  // Get direct power usage for a specific device
  Result get_device_power_usage_direct(size_t device_index,
                                       double& power_watts);

  // Get current energy consumption
  Result get_current_energy_consumption(size_t device_index,
                                        double& energy_joules);

  // Get number of available devices
  size_t get_device_count() const;

  // Get device name
  std::string get_device_name(size_t device_index) const;

  // Check if provider is in working state
  bool is_initialized() const { return is_initialized_; }

 private:
  static constexpr const char* COMPONENT_NAME = "NVMLProvider";

  bool is_initialized_;
  std::vector<nvmlDevice_t> devices_;
  std::vector<std::string> device_names_;

  // Helper methods
  Result discover_devices();
  void cleanup_devices();
  Result validate_device_index(size_t device_index) const;
  std::string nvml_error_to_string(nvmlReturn_t result) const;
};

}  // namespace EnergyProfiler
}  // namespace KokkosTools
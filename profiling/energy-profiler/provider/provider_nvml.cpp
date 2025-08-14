#include "provider_nvml.hpp"
#include <cstring>

namespace KokkosTools {
namespace EnergyProfiler {

NVMLProvider::NVMLProvider() : is_initialized_(false) {}

NVMLProvider::~NVMLProvider() {
  if (is_initialized_) {
    finalize();
  }
}

Result NVMLProvider::initialize() {
  if (is_initialized_) {
    return Result(ErrorCode::SUCCESS);
  }

  // Initialize NVML
  nvmlReturn_t nvml_result = nvmlInit();
  if (NVML_SUCCESS != nvml_result) {
    std::string error_msg =
        "Failed to initialize NVML: " + nvml_error_to_string(nvml_result);
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::PROVIDER_INIT_FAILED, error_msg);
  }

  // Discover devices
  Result discover_result = discover_devices();
  if (!discover_result) {
    nvmlShutdown();
    return discover_result;
  }

  is_initialized_ = true;
  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "Successfully initialized with " +
                                               std::to_string(devices_.size()) +
                                               " device(s)");

  return Result(ErrorCode::SUCCESS);
}

void NVMLProvider::finalize() {
  if (!is_initialized_) {
    return;
  }

  cleanup_devices();
  nvmlShutdown();
  is_initialized_ = false;

  ENERGY_PROFILER_LOG_INFO(COMPONENT_NAME, "Finalized");
}

Result NVMLProvider::get_total_power_usage(double& power_watts) {
  if (!is_initialized_) {
    return Result(ErrorCode::PROVIDER_INIT_FAILED, "Provider not initialized");
  }

  power_watts = 0.0;

  for (size_t i = 0; i < devices_.size(); ++i) {
    double device_power = 0.0;
    Result result       = get_device_power_usage(i, device_power);
    if (result) {
      power_watts += device_power;
    }
  }

  return Result(ErrorCode::SUCCESS);
}

Result NVMLProvider::get_device_power_usage(size_t device_index,
                                            double& power_watts) {
  Result validation = validate_device_index(device_index);
  if (!validation) {
    return validation;
  }

  if (devices_[device_index] == nullptr) {
    return Result(ErrorCode::DEVICE_ACCESS_FAILED, "Device handle is null");
  }

  unsigned int power_mW = 0;
  nvmlReturn_t nvml_result =
      nvmlDeviceGetPowerUsage(devices_[device_index], &power_mW);

  if (nvml_result == NVML_SUCCESS) {
    // Convert from milliwatts to watts
    power_watts = static_cast<double>(power_mW) / 1000.0;
    return Result(ErrorCode::SUCCESS);
  } else {
    std::string error_msg = "Failed to get power usage for device " +
                            std::to_string(device_index) + ": " +
                            nvml_error_to_string(nvml_result);
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::MEASUREMENT_FAILED, error_msg);
  }
}

Result NVMLProvider::get_device_power_usage_direct(size_t device_index,
                                                   double& power_watts) {
  Result validation = validate_device_index(device_index);
  if (!validation) {
    return validation;
  }

  if (devices_[device_index] == nullptr) {
    return Result(ErrorCode::DEVICE_ACCESS_FAILED, "Device handle is null");
  }

  nvmlFieldValue_t power_field_now;
  power_field_now.fieldId = NVML_FI_DEV_POWER_INSTANT;
  nvmlReturn_t nvml_result =
      nvmlDeviceGetFieldValues(devices_[device_index], 1, &power_field_now);

  if (nvml_result != NVML_SUCCESS) {
    std::string error_msg =
        "NVML direct power read failed: " + nvml_error_to_string(nvml_result);
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::MEASUREMENT_FAILED, error_msg);
  }

  unsigned int pw = static_cast<unsigned int>(power_field_now.value.uiVal);
  // Convert from milliwatts to watts
  power_watts = static_cast<double>(pw) / 1000.0;
  return Result(ErrorCode::SUCCESS);
}

Result NVMLProvider::get_current_energy_consumption(size_t device_index,
                                                    double& energy_joules) {
  Result validation = validate_device_index(device_index);
  if (!validation) {
    return validation;
  }

  if (devices_[device_index] == nullptr) {
    return Result(ErrorCode::DEVICE_ACCESS_FAILED, "Device handle is null");
  }

  unsigned long long energy_mJ = 0;
  nvmlReturn_t nvml_result =
      nvmlDeviceGetTotalEnergyConsumption(devices_[device_index], &energy_mJ);

  if (nvml_result == NVML_SUCCESS) {
    // Convert from millijoules to joules
    energy_joules = static_cast<double>(energy_mJ) / 1000.0;
    return Result(ErrorCode::SUCCESS);
  } else {
    std::string error_msg = "Failed to get energy consumption for device " +
                            std::to_string(device_index) + ": " +
                            nvml_error_to_string(nvml_result);
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::MEASUREMENT_FAILED, error_msg);
  }
}

size_t NVMLProvider::get_device_count() const { return devices_.size(); }

std::string NVMLProvider::get_device_name(size_t device_index) const {
  if (device_index >= device_names_.size()) {
    return "Unknown Device";
  }
  return device_names_[device_index];
}

Result NVMLProvider::discover_devices() {
  unsigned int device_count;
  nvmlReturn_t nvml_result = nvmlDeviceGetCount(&device_count);

  if (NVML_SUCCESS != nvml_result) {
    std::string error_msg =
        "Failed to get device count: " + nvml_error_to_string(nvml_result);
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::DEVICE_ACCESS_FAILED, error_msg);
  }

  if (device_count == 0) {
    std::string error_msg = "No NVIDIA devices found";
    ENERGY_PROFILER_LOG_ERROR(COMPONENT_NAME, error_msg);
    return Result(ErrorCode::DEVICE_ACCESS_FAILED, error_msg);
  }

  devices_.resize(device_count);
  device_names_.resize(device_count);

  ENERGY_PROFILER_LOG_INFO(
      COMPONENT_NAME,
      "Found " + std::to_string(device_count) + " NVIDIA device(s)");

  for (unsigned int i = 0; i < device_count; ++i) {
    nvml_result = nvmlDeviceGetHandleByIndex(i, &devices_[i]);
    if (NVML_SUCCESS != nvml_result) {
      ENERGY_PROFILER_LOG_WARNING(
          COMPONENT_NAME,
          "Failed to get handle for device " + std::to_string(i));
      devices_[i]      = nullptr;
      device_names_[i] = "Failed Device";
      continue;
    }

    // Get device name
    char device_name[NVML_DEVICE_NAME_BUFFER_SIZE];
    nvml_result = nvmlDeviceGetName(devices_[i], device_name,
                                    NVML_DEVICE_NAME_BUFFER_SIZE);
    if (NVML_SUCCESS == nvml_result) {
      device_names_[i] = std::string(device_name);
      ENERGY_PROFILER_LOG_INFO(
          COMPONENT_NAME, "Device " + std::to_string(i) + ": " + device_name);
    } else {
      device_names_[i] = "Unknown Device " + std::to_string(i);
    }

    // Check power management capability
    nvmlEnableState_t pm_mode;
    nvml_result = nvmlDeviceGetPowerManagementMode(devices_[i], &pm_mode);
    if (NVML_SUCCESS == nvml_result && pm_mode == NVML_FEATURE_ENABLED) {
      ENERGY_PROFILER_LOG_INFO(
          COMPONENT_NAME,
          "Device " + std::to_string(i) + ": Power management enabled");
    } else {
      ENERGY_PROFILER_LOG_WARNING(
          COMPONENT_NAME, "Device " + std::to_string(i) +
                              ": Power management disabled or not supported");
    }

    // Test power usage reading
    unsigned int test_power_mW = 0;
    nvml_result = nvmlDeviceGetPowerUsage(devices_[i], &test_power_mW);
    if (NVML_SUCCESS == nvml_result) {
      ENERGY_PROFILER_LOG_INFO(
          COMPONENT_NAME, "Device " + std::to_string(i) +
                              ": Current power usage: " +
                              std::to_string(test_power_mW / 1000.0) + " W");
    } else {
      ENERGY_PROFILER_LOG_WARNING(COMPONENT_NAME,
                                  "Device " + std::to_string(i) +
                                      ": Power usage reading failed: " +
                                      nvml_error_to_string(nvml_result));
    }
  }

  return Result(ErrorCode::SUCCESS);
}

void NVMLProvider::cleanup_devices() {
  devices_.clear();
  device_names_.clear();
}

Result NVMLProvider::validate_device_index(size_t device_index) const {
  if (!is_initialized_) {
    return Result(ErrorCode::PROVIDER_INIT_FAILED, "Provider not initialized");
  }

  if (device_index >= devices_.size()) {
    return Result(ErrorCode::INVALID_DEVICE_INDEX,
                  "Device index " + std::to_string(device_index) +
                      " out of range (max: " + std::to_string(devices_.size()) +
                      ")");
  }

  return Result(ErrorCode::SUCCESS);
}

std::string NVMLProvider::nvml_error_to_string(nvmlReturn_t result) const {
  return std::string(nvmlErrorString(result));
}

}  // namespace EnergyProfiler
}  // namespace KokkosTools
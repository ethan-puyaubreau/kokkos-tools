#include <iostream>
#include <chrono>
#include <thread>
#include "../provider/provider_nvml.hpp"

using namespace KokkosTools::EnergyProfiler;

void test_nvml_provider() {
  std::cout << "=== NVML Provider Test ===" << std::endl;

  NVMLProvider provider;

  // Test initialization
  std::cout << "\n1. Testing initialization..." << std::endl;
  Result init_result = provider.initialize();
  if (!init_result.is_success()) {
    std::cout << "ERROR: Failed to initialize NVML provider: "
              << init_result.message << std::endl;
    return;
  }
  std::cout << "SUCCESS: NVML provider initialized successfully" << std::endl;

  // Test device discovery
  std::cout << "\n2. Testing device discovery..." << std::endl;
  size_t device_count = provider.get_device_count();
  std::cout << "Found " << device_count << " device(s)" << std::endl;

  if (device_count == 0) {
    std::cout << "ERROR: No devices found" << std::endl;
    return;
  }

  // Display device information
  std::cout << "\n3. Device information:" << std::endl;
  for (size_t i = 0; i < device_count; ++i) {
    std::string name = provider.get_device_name(i);
    std::cout << "  Device " << i << ": " << name << std::endl;
  }

  // Test power readings
  std::cout << "\n4. Testing power readings..." << std::endl;
  for (int sample = 0; sample < 5; ++sample) {
    std::cout << "Sample " << (sample + 1) << ":" << std::endl;

    // Individual device power
    for (size_t i = 0; i < device_count; ++i) {
      double power        = 0.0;
      Result power_result = provider.get_device_power_usage(i, power);
      if (power_result.is_success()) {
        std::cout << "  Device " << i << ": " << power << " W" << std::endl;
      } else {
        std::cout << "  Device " << i
                  << ": Failed to read power: " << power_result.message
                  << std::endl;
      }
    }

    // Individual device direct power
    for (size_t i = 0; i < device_count; ++i) {
      double direct_power = 0.0;
      Result direct_result =
          provider.get_device_power_usage_direct(i, direct_power);
      if (direct_result.is_success()) {
        std::cout << "  Device " << i << " (Direct): " << direct_power << " W"
                  << std::endl;
      } else {
        std::cout << "  Device " << i
                  << " (Direct): Failed to read direct power: "
                  << direct_result.message << std::endl;
      }
    }

    // Current energy consumption
    for (size_t i = 0; i < device_count; ++i) {
      double energy        = 0.0;
      Result energy_result = provider.get_current_energy_consumption(i, energy);
      if (energy_result.is_success()) {
        std::cout << "  Device " << i << " Energy: " << energy << " J"
                  << std::endl;
      } else {
        std::cout << "  Device " << i
                  << " Energy: Failed to read energy: " << energy_result.message
                  << std::endl;
      }
    }

    // Total power
    double total_power  = 0.0;
    Result total_result = provider.get_total_power_usage(total_power);
    if (total_result.is_success()) {
      std::cout << "  Total Power: " << total_power << " W" << std::endl;
    } else {
      std::cout << "  Total Power: Failed to read: " << total_result.message
                << std::endl;
    }

    if (sample < 4) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  // Test finalization
  std::cout << "\n5. Testing finalization..." << std::endl;
  provider.finalize();
  std::cout << "SUCCESS: NVML provider finalized successfully" << std::endl;

  std::cout << "\n=== Test Completed ===" << std::endl;
}

int main() {
  try {
    test_nvml_provider();
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "ERROR: Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}
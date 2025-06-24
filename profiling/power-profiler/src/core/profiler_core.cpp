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

#include "profiler_core.hpp"
#include "../providers/energy_provider.hpp"
#include <iostream>

namespace KokkosTools {
namespace PowerProfiler {

PowerProfilerCore::PowerProfilerCore(ProfilerConfig& config)
    : config_(config),
      timing_manager_(std::make_unique<TimingManager>()),
      correlator_(std::make_unique<EnergyCorrelator>()),
      initialization_successful_(false) {
  config_.load_from_environment();
}

bool PowerProfilerCore::initialize() {
  if (!setup_energy_monitoring()) {
    if (config_.fail_on_provider_error) {
      std::cerr << "PowerProfiler: Failed to initialize energy provider and "
                   "fail_on_provider_error is set.\n";
      return false;
    } else {
      std::cout << "PowerProfiler: Continuing without energy monitoring.\n";
    }
  }

  setup_output_handler();

  if (power_monitor_ && power_monitor_->start()) {
    initialization_successful_ = true;
  }

  return true;
}

void PowerProfilerCore::finalize() {
  if (power_monitor_) {
    power_monitor_->stop();

    if (power_monitor_->has_error()) {
      std::cerr << "PowerProfiler: Energy monitoring had errors: "
                << power_monitor_->get_last_error() << "\n";
    }
  }

  perform_analysis_and_output();
}

void PowerProfilerCore::begin_kernel(uint64_t kernel_id,
                                     const std::string& name, KernelType type, uint32_t device_id) {
  if (timing_manager_) {
    timing_manager_->begin_kernel(kernel_id, name, type, device_id);
  }
}

void PowerProfilerCore::end_kernel(uint64_t kernel_id) {
  if (timing_manager_) {
    timing_manager_->end_kernel(kernel_id);
  }
}

void PowerProfilerCore::push_region(const std::string& name) {
  if (timing_manager_) {
    timing_manager_->push_region(name);
  }
}

void PowerProfilerCore::pop_region() {
  if (timing_manager_) {
    timing_manager_->pop_region();
  }
}

bool PowerProfilerCore::setup_energy_monitoring() {
  try {
    std::unique_ptr<EnergyProvider> provider;

    if (config_.energy_provider_type ==
        ProfilerConfig::ProviderType::VARIORUM) {
      provider = EnergyProviderFactory::create(
          EnergyProviderFactory::ProviderType::VARIORUM);
    } else if (config_.energy_provider_type ==
               ProfilerConfig::ProviderType::DUMMY) {
      provider = EnergyProviderFactory::create(
          EnergyProviderFactory::ProviderType::DUMMY);
    } else {
      // Test: Try auto-detection
      provider = EnergyProviderFactory::create_best_available();
    }

    if (provider) {
      power_monitor_ = std::make_unique<PowerMonitor>(
          std::move(provider), config_.monitor_interval,
          config_.initial_data_capacity);
      std::cout << "PowerProfiler: Energy provider initialized successfully, "
                   "with following configuration:\n"
                << config_ << "\n";
      return true;
    }

    return false;
  } catch (const std::exception& e) {
    std::cerr << "PowerProfiler: Failed to create energy provider: " << e.what()
              << "\n";
    return false;
  }
}

void PowerProfilerCore::setup_output_handler() {
  try {
    if (config_.output_type == ProfilerConfig::OutputType::CONSOLE) {
      output_handler_ = OutputHandlerFactory::create(
          OutputHandlerFactory::Format::CONSOLE);
    } else if (config_.output_type == ProfilerConfig::OutputType::JSON_FILE) {
      output_handler_ = OutputHandlerFactory::create(
          OutputHandlerFactory::Format::JSON,
          config_.output_file_path);
    } else if (config_.output_type == ProfilerConfig::OutputType::CSV_FILE) {
      output_handler_ = OutputHandlerFactory::create(
          OutputHandlerFactory::Format::CSV,
          config_.output_file_path);
    }
  } catch (const std::exception& e) {
    std::cerr << "PowerProfiler: Failed to create output handler: " << e.what()
              << ". Falling back to console output."
              << "\n";
    output_handler_ = OutputHandlerFactory::create(
        OutputHandlerFactory::Format::CONSOLE);
  }
}

void PowerProfilerCore::perform_analysis_and_output() {
  if (!output_handler_) return;

  try {
    // Get all the raw timing data
    auto kernel_timings = timing_manager_ ? timing_manager_->get_kernel_timings() 
                                          : std::vector<KernelTiming>();
    auto region_timings = timing_manager_ ? timing_manager_->get_region_timings() 
                                          : std::vector<RegionTiming>();

    // Collect and output raw power data
    std::vector<EnergyReading> energy_readings;
    if (power_monitor_) {
      energy_readings = power_monitor_->get_collected_data();
      output_handler_->output_power_data(energy_readings);
    }
    
    // Calculate energy correlations if we have energy data
    std::vector<KernelEnergyCorrelation> kernel_correlations;
    std::vector<RegionEnergyCorrelation> region_correlations;
    
    if (!energy_readings.empty() && correlator_) {
      if (!kernel_timings.empty()) {
        kernel_correlations = correlator_->correlate_kernels_with_energy(
            kernel_timings, energy_readings);
      }
      
      if (!region_timings.empty()) {
        region_correlations = correlator_->correlate_regions_with_energy(
            region_timings, energy_readings, kernel_timings);
      }
    }
    
    // Output structured data for kernels and regions
    output_handler_->output_kernel_data(kernel_timings, 
                                      !kernel_correlations.empty() ? &kernel_correlations : nullptr);
    output_handler_->output_region_data(region_timings, 
                                      !region_correlations.empty() ? &region_correlations : nullptr);
  } catch (const std::exception& e) {
    std::cerr << "PowerProfiler: Error during output: " << e.what() << "\n";
  }
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

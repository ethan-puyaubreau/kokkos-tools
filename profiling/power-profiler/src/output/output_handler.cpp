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

#include "output_handler.hpp"
#include "../data/execution_space_stats.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace KokkosTools {
namespace PowerProfiler {

// Console Output Handler
void ConsoleOutputHandler::output_power_data(
    const std::vector<EnergyReading>& readings) {
  std::cout << "\n--- POWER_DATA_START ---\n";
  std::cout
      << "DATA TYPE : POWER_READING, TIMESTAMP_MS, DEVICE_ID, POWER_WATTS\n";
  if (readings.empty()) {
    std::cout << "POWER_READING_NONE\n";
  } else {
    for (const auto& reading : readings) {
      for (const auto& pair : reading.device_powers) {
        std::cout << "POWER_READING," << reading.timestamp_ms() << ","
                  << pair.first << "," << std::fixed << std::setprecision(2)
                  << pair.second << '\n';
      }
    }
  }
  std::cout << "--- POWER_DATA_END ---\n";
}

void ConsoleOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings) {
  std::cout << "\n--- KERNEL_DATA_START ---\n";
  std::cout << "DATA TYPE : KERNEL_TIMING, TYPE, NAME, EXECUTION_SPACE, DEVICE_ID, START_TIME_MS, "
               "END_TIME_MS, DURATION_MS\n";
  if (timings.empty()) {
    std::cout << "KERNEL_TIMING_NONE\n";
  } else {
    for (const auto& timing : timings) {
      std::cout << "KERNEL_TIMING," << kernel_type_to_string(timing.type)
                << ",\"" << timing.name << "\"," << execution_space_to_string(timing.execution_space)
                << "," << timing.device_id << "," << timing.start_time_ms()
                << "," << timing.end_time_ms() << ","
                << timing.duration_ms_value() << '\n';
    }
  }
  std::cout << "--- KERNEL_DATA_END ---\n";
}

void ConsoleOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings) {
  std::cout << "\n--- REGION_DATA_START ---\n";
  std::cout << "DATA TYPE : REGION_TIMING, NAME, START_TIME_MS, END_TIME_MS, "
               "DURATION_MS\n";
  if (timings.empty()) {
    std::cout << "REGION_TIMING_NONE\n";
  } else {
    for (const auto& timing : timings) {
      std::cout << "REGION_TIMING,\"" << timing.name << "\","
                << timing.start_time_ms() << "," << timing.end_time_ms() << ","
                << timing.duration_ms_value() << '\n';
    }
  }
  std::cout << "--- REGION_DATA_END ---\n";
}

void ConsoleOutputHandler::output_kernel_correlations(
    const std::vector<KernelEnergyCorrelation>& correlations) {
  std::cout << "\n--- ENERGY_CORRELATION_START ---\n";
  std::cout << "DATA TYPE : KERNEL_ENERGY, NAME, EXECUTION_SPACE, DEVICE_ID, DURATION_MS, "
               "TOTAL_ENERGY_JOULES, AVERAGE_POWER_WATTS\n";
  if (correlations.empty()) {
    std::cout << "KERNEL_ENERGY_NONE\n";
  } else {
    for (const auto& corr : correlations) {
      std::cout << "KERNEL_ENERGY,\"" << corr.kernel.name << "\","
                << execution_space_to_string(corr.kernel.execution_space)
                << "," << corr.kernel.device_id << ","
                << corr.kernel.duration_ms_value() << "," << std::fixed
                << std::setprecision(6) << corr.total_energy_joules << ","
                << corr.average_power_watts << '\n';
    }
  }
  std::cout << "--- ENERGY_CORRELATION_END ---\n";
}

void ConsoleOutputHandler::output_region_correlations(
    const std::vector<RegionEnergyCorrelation>& correlations) {
  std::cout << "\n--- REGION_ENERGY_CORRELATION_START ---\n";
  std::cout << "DATA TYPE : REGION_ENERGY, NAME, DURATION_MS, "
               "TOTAL_ENERGY_JOULES, AVERAGE_POWER_WATTS\n";
  if (correlations.empty()) {
    std::cout << "REGION_ENERGY_NONE\n";
  } else {
    for (const auto& corr : correlations) {
      std::cout << "REGION_ENERGY,\"" << corr.region.name << "\","
                << corr.region.duration_ms_value() << "," << std::fixed
                << std::setprecision(6) << corr.total_energy_joules << ","
                << corr.average_power_watts << '\n';
    }
  }
  std::cout << "--- REGION_ENERGY_CORRELATION_END ---\n";
}

void ConsoleOutputHandler::output_execution_space_stats(
    const std::map<ExecutionSpace, ExecutionSpaceStats>& stats) {
  std::cout << "\n--- EXECUTION_SPACE_STATS_START ---\n";
  std::cout << "DATA TYPE : EXECUTION_SPACE, NAME, IS_HOST, IS_DEVICE, "
               "KERNEL_COUNT, TOTAL_DURATION_MS, AVG_DURATION_MS, "
               "MIN_DURATION_MS, MAX_DURATION_MS, TOTAL_ENERGY_JOULES, "
               "AVG_POWER_WATTS, MIN_POWER_WATTS, MAX_POWER_WATTS\n";
  
  if (stats.empty()) {
    std::cout << "EXECUTION_SPACE_NONE\n";
  } else {
    for (const auto& pair : stats) {
      const auto& space_stats = pair.second;
      std::cout << "EXECUTION_SPACE," << space_stats.space_name << ","
                << (space_stats.is_host ? "true" : "false") << ","
                << (space_stats.is_device ? "true" : "false") << ","
                << space_stats.kernel_count << ","
                << space_stats.total_duration_ms << "," << std::fixed
                << std::setprecision(2) << space_stats.average_duration_ms << ","
                << space_stats.min_duration_ms << ","
                << space_stats.max_duration_ms << "," << std::fixed
                << std::setprecision(6) << space_stats.total_energy_joules << ","
                << std::setprecision(2) << space_stats.average_power_watts << ","
                << space_stats.min_power_watts << ","
                << space_stats.max_power_watts << '\n';
    }
  }
  std::cout << "--- EXECUTION_SPACE_STATS_END ---\n";
  
  // Create and display host vs device summary
  auto summary = ExecutionSpaceAnalyzer::create_host_vs_device_summary(stats);
  std::cout << "\n--- HOST_VS_DEVICE_SUMMARY_START ---\n";
  std::cout << "DATA TYPE : SUMMARY, TYPE, KERNEL_COUNT, TOTAL_DURATION_MS, "
               "AVG_DURATION_MS, TOTAL_ENERGY_JOULES, AVG_POWER_WATTS\n";
  
  if (summary.first.kernel_count > 0) {
    const auto& host = summary.first;
    std::cout << "SUMMARY,HOST," << host.kernel_count << ","
              << host.total_duration_ms << "," << std::fixed
              << std::setprecision(2) << host.average_duration_ms << ","
              << std::setprecision(6) << host.total_energy_joules << ","
              << std::setprecision(2) << host.average_power_watts << '\n';
  }
  
  if (summary.second.kernel_count > 0) {
    const auto& device = summary.second;
    std::cout << "SUMMARY,DEVICE," << device.kernel_count << ","
              << device.total_duration_ms << "," << std::fixed
              << std::setprecision(2) << device.average_duration_ms << ","
              << std::setprecision(6) << device.total_energy_joules << ","
              << std::setprecision(2) << device.average_power_watts << '\n';
  }
  
  std::cout << "--- HOST_VS_DEVICE_SUMMARY_END ---\n";
}

// JSON Output Handler
JsonOutputHandler::JsonOutputHandler(const std::string& file_path)
    : file_path_(file_path) {}

void JsonOutputHandler::output_power_data(
    const std::vector<EnergyReading>& readings) {
  std::ostringstream json;
  json << "{\n  \"power_readings\": [\n";

  for (size_t i = 0; i < readings.size(); ++i) {
    const auto& reading = readings[i];
    json << "    {\n";
    json << "      \"timestamp_ms\": " << reading.timestamp_ms() << ",\n";
    json << "      \"devices\": {\n";

    size_t device_count = 0;
    for (const auto& pair : reading.device_powers) {
      json << "        \"" << pair.first << "\": " << std::fixed
           << std::setprecision(2) << pair.second;
      if (++device_count < reading.device_powers.size()) json << ",";
      json << "\n";
    }

    json << "      }\n    }";
    if (i < readings.size() - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n}";
  write_to_file(json.str());
}

void JsonOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings) {
  std::ostringstream json;
  json << "{\n  \"kernel_timings\": [\n";

  for (size_t i = 0; i < timings.size(); ++i) {
    const auto& timing = timings[i];
    json << "    {\n";
    json << "      \"name\": \"" << timing.name << "\",\n";
    json << "      \"type\": \"" << kernel_type_to_string(timing.type)
         << "\",\n";
    json << "      \"execution_space\": \"" << execution_space_to_string(timing.execution_space)
         << "\",\n";
    json << "      \"device_id\": " << timing.device_id << ",\n";
    json << "      \"instance_id\": " << timing.instance_id << ",\n";
    json << "      \"is_host_execution\": " << (is_host_execution_space(timing.execution_space) ? "true" : "false") << ",\n";
    json << "      \"is_device_execution\": " << (is_device_execution_space(timing.execution_space) ? "true" : "false") << ",\n";
    json << "      \"start_time_ms\": " << timing.start_time_ms() << ",\n";
    json << "      \"end_time_ms\": " << timing.end_time_ms() << ",\n";
    json << "      \"duration_ms\": " << timing.duration_ms_value() << "\n";
    json << "    }";
    if (i < timings.size() - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n}";
  write_to_file(json.str());
}

void JsonOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings) {
  std::ostringstream json;
  json << "{\n  \"region_timings\": [\n";

  for (size_t i = 0; i < timings.size(); ++i) {
    const auto& timing = timings[i];
    json << "    {\n";
    json << "      \"name\": \"" << timing.name << "\",\n";
    json << "      \"start_time_ms\": " << timing.start_time_ms() << ",\n";
    json << "      \"end_time_ms\": " << timing.end_time_ms() << ",\n";
    json << "      \"duration_ms\": " << timing.duration_ms_value() << "\n";
    json << "    }";
    if (i < timings.size() - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n}";
  write_to_file(json.str());
}

void JsonOutputHandler::output_kernel_correlations(
    const std::vector<KernelEnergyCorrelation>& correlations) {
  std::ostringstream json;
  json << "{\n  \"kernel_energy_correlations\": [\n";

  for (size_t i = 0; i < correlations.size(); ++i) {
    const auto& corr = correlations[i];
    json << "    {\n";
    json << "      \"name\": \"" << corr.kernel.name << "\",\n";
    json << "      \"type\": \"" << kernel_type_to_string(corr.kernel.type)
         << "\",\n";
    json << "      \"execution_space\": \"" << execution_space_to_string(corr.kernel.execution_space)
         << "\",\n";
    json << "      \"device_id\": " << corr.kernel.device_id << ",\n";
    json << "      \"instance_id\": " << corr.kernel.instance_id << ",\n";
    json << "      \"is_host_execution\": " << (is_host_execution_space(corr.kernel.execution_space) ? "true" : "false") << ",\n";
    json << "      \"is_device_execution\": " << (is_device_execution_space(corr.kernel.execution_space) ? "true" : "false") << ",\n";
    json << "      \"duration_ms\": " << corr.kernel.duration_ms_value()
         << ",\n";
    json << "      \"total_energy_joules\": " << std::fixed
         << std::setprecision(6) << corr.total_energy_joules << ",\n";
    json << "      \"average_power_watts\": " << corr.average_power_watts
         << "\n";
    json << "    }";
    if (i < correlations.size() - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n}";
  write_to_file(json.str());
}

void JsonOutputHandler::output_region_correlations(
    const std::vector<RegionEnergyCorrelation>& correlations) {
  std::ostringstream json;
  json << "{\n  \"region_energy_correlations\": [\n";

  for (size_t i = 0; i < correlations.size(); ++i) {
    const auto& corr = correlations[i];
    json << "    {\n";
    json << "      \"name\": \"" << corr.region.name << "\",\n";
    json << "      \"duration_ms\": " << corr.region.duration_ms_value()
         << ",\n";
    json << "      \"total_energy_joules\": " << std::fixed
         << std::setprecision(6) << corr.total_energy_joules << ",\n";
    json << "      \"average_power_watts\": " << corr.average_power_watts
         << ",\n";
    json << "      \"kernels_count\": " << corr.kernels_in_region.size()
         << "\n";
    json << "    }";
    if (i < correlations.size() - 1) json << ",";
    json << "\n";
  }

  json << "  ]\n}";
  write_to_file(json.str());
}

void JsonOutputHandler::output_execution_space_stats(
    const std::map<ExecutionSpace, ExecutionSpaceStats>& stats) {
  std::ostringstream json;
  json << "{\n  \"execution_space_stats\": [\n";

  size_t i = 0;
  for (const auto& pair : stats) {
    const auto& space_stats = pair.second;
    json << "    {\n";
    json << "      \"execution_space\": \"" << space_stats.space_name << "\",\n";
    json << "      \"is_host\": " << (space_stats.is_host ? "true" : "false") << ",\n";
    json << "      \"is_device\": " << (space_stats.is_device ? "true" : "false") << ",\n";
    json << "      \"kernel_count\": " << space_stats.kernel_count << ",\n";
    json << "      \"total_duration_ms\": " << space_stats.total_duration_ms << ",\n";
    json << "      \"average_duration_ms\": " << std::fixed << std::setprecision(2) 
         << space_stats.average_duration_ms << ",\n";
    json << "      \"min_duration_ms\": " << space_stats.min_duration_ms << ",\n";
    json << "      \"max_duration_ms\": " << space_stats.max_duration_ms << ",\n";
    json << "      \"total_energy_joules\": " << std::setprecision(6) 
         << space_stats.total_energy_joules << ",\n";
    json << "      \"average_power_watts\": " << std::setprecision(2) 
         << space_stats.average_power_watts << ",\n";
    json << "      \"min_power_watts\": " << space_stats.min_power_watts << ",\n";
    json << "      \"max_power_watts\": " << space_stats.max_power_watts << "\n";
    json << "    }";
    if (i < stats.size() - 1) json << ",";
    json << "\n";
    i++;
  }

  json << "  ],\n";
  
  // Add host vs device summary
  auto summary = ExecutionSpaceAnalyzer::create_host_vs_device_summary(stats);
  json << "  \"host_vs_device_summary\": {\n";
  json << "    \"host\": {\n";
  json << "      \"kernel_count\": " << summary.first.kernel_count << ",\n";
  json << "      \"total_duration_ms\": " << summary.first.total_duration_ms << ",\n";
  json << "      \"average_duration_ms\": " << std::fixed << std::setprecision(2) 
       << summary.first.average_duration_ms << ",\n";
  json << "      \"total_energy_joules\": " << std::setprecision(6) 
       << summary.first.total_energy_joules << ",\n";
  json << "      \"average_power_watts\": " << std::setprecision(2) 
       << summary.first.average_power_watts << "\n";
  json << "    },\n";
  json << "    \"device\": {\n";
  json << "      \"kernel_count\": " << summary.second.kernel_count << ",\n";
  json << "      \"total_duration_ms\": " << summary.second.total_duration_ms << ",\n";
  json << "      \"average_duration_ms\": " << std::fixed << std::setprecision(2) 
       << summary.second.average_duration_ms << ",\n";
  json << "      \"total_energy_joules\": " << std::setprecision(6) 
       << summary.second.total_energy_joules << ",\n";
  json << "      \"average_power_watts\": " << std::setprecision(2) 
       << summary.second.average_power_watts << "\n";
  json << "    }\n";
  json << "  }\n";
  json << "}";

  write_to_file(json.str());
}

void JsonOutputHandler::write_to_file(const std::string& json_content) {
  std::ofstream file(file_path_, std::ios::app);
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file: " + file_path_);
  }
  file << json_content << "\n";
}

// CSV Output Handler
CsvOutputHandler::CsvOutputHandler(const std::string& file_path_prefix)
    : file_path_prefix_(file_path_prefix) {}

void CsvOutputHandler::output_power_data(
    const std::vector<EnergyReading>& readings) {
  std::ofstream file(file_path_prefix_ + "_power.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open power CSV file");
  }

  file << "timestamp_ms,device_id,power_watts\n";
  for (const auto& reading : readings) {
    for (const auto& pair : reading.device_powers) {
      file << reading.timestamp_ms() << "," << pair.first << "," << std::fixed
           << std::setprecision(2) << pair.second << "\n";
    }
  }
}

void CsvOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings) {
  std::ofstream file(file_path_prefix_ + "_kernels.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open kernel CSV file");
  }

  file << "name,type,execution_space,device_id,instance_id,is_host_execution,is_device_execution,start_time_ms,end_time_ms,duration_ms\n";
  for (const auto& timing : timings) {
    file << "\"" << timing.name << "\"," << kernel_type_to_string(timing.type)
         << "," << execution_space_to_string(timing.execution_space)
         << "," << timing.device_id << "," << timing.instance_id
         << "," << (is_host_execution_space(timing.execution_space) ? "true" : "false")
         << "," << (is_device_execution_space(timing.execution_space) ? "true" : "false")
         << "," << timing.start_time_ms() << "," << timing.end_time_ms() << ","
         << timing.duration_ms_value() << "\n";
  }
}

void CsvOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings) {
  std::ofstream file(file_path_prefix_ + "_regions.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open region CSV file");
  }

  file << "name,start_time_ms,end_time_ms,duration_ms\n";
  for (const auto& timing : timings) {
    file << "\"" << timing.name << "\"," << timing.start_time_ms() << ","
         << timing.end_time_ms() << "," << timing.duration_ms_value() << "\n";
  }
}

void CsvOutputHandler::output_kernel_correlations(
    const std::vector<KernelEnergyCorrelation>& correlations) {
  std::ofstream file(file_path_prefix_ + "_kernel_energy.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open kernel energy CSV file");
  }

  file << "name,type,execution_space,device_id,instance_id,is_host_execution,is_device_execution,duration_ms,total_energy_joules,average_power_watts\n";
  for (const auto& corr : correlations) {
    file << "\"" << corr.kernel.name << "\","
         << kernel_type_to_string(corr.kernel.type) << ","
         << execution_space_to_string(corr.kernel.execution_space) << ","
         << corr.kernel.device_id << "," << corr.kernel.instance_id << ","
         << (is_host_execution_space(corr.kernel.execution_space) ? "true" : "false") << ","
         << (is_device_execution_space(corr.kernel.execution_space) ? "true" : "false") << ","
         << corr.kernel.duration_ms_value() << "," << std::fixed
         << std::setprecision(6) << corr.total_energy_joules << ","
         << corr.average_power_watts << "\n";
  }
}

void CsvOutputHandler::output_region_correlations(
    const std::vector<RegionEnergyCorrelation>& correlations) {
  std::ofstream file(file_path_prefix_ + "_region_energy.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open region energy CSV file");
  }

  file << "name,duration_ms,total_energy_joules,average_power_watts,kernels_"
          "count\n";
  for (const auto& corr : correlations) {
    file << "\"" << corr.region.name << "\"," << corr.region.duration_ms_value()
         << "," << std::fixed << std::setprecision(6)
         << corr.total_energy_joules << "," << corr.average_power_watts << ","
         << corr.kernels_in_region.size() << "\n";
  }
}

void CsvOutputHandler::output_execution_space_stats(
    const std::map<ExecutionSpace, ExecutionSpaceStats>& stats) {
  // Output detailed execution space statistics
  std::ofstream file(file_path_prefix_ + "_execution_space_stats.csv");
  if (!file.is_open()) {
    throw std::runtime_error("Failed to open execution space stats CSV file");
  }

  file << "execution_space,is_host,is_device,kernel_count,total_duration_ms,"
          "average_duration_ms,min_duration_ms,max_duration_ms,"
          "total_energy_joules,average_power_watts,min_power_watts,max_power_watts\n";
  
  for (const auto& pair : stats) {
    const auto& space_stats = pair.second;
    file << space_stats.space_name << ","
         << (space_stats.is_host ? "true" : "false") << ","
         << (space_stats.is_device ? "true" : "false") << ","
         << space_stats.kernel_count << ","
         << space_stats.total_duration_ms << "," << std::fixed
         << std::setprecision(2) << space_stats.average_duration_ms << ","
         << space_stats.min_duration_ms << ","
         << space_stats.max_duration_ms << "," << std::setprecision(6)
         << space_stats.total_energy_joules << "," << std::setprecision(2)
         << space_stats.average_power_watts << ","
         << space_stats.min_power_watts << ","
         << space_stats.max_power_watts << "\n";
  }
  
  // Output host vs device summary in a separate file
  std::ofstream summary_file(file_path_prefix_ + "_host_vs_device_summary.csv");
  if (!summary_file.is_open()) {
    throw std::runtime_error("Failed to open host vs device summary CSV file");
  }

  summary_file << "type,kernel_count,total_duration_ms,average_duration_ms,"
                  "total_energy_joules,average_power_watts\n";
  
  auto summary = ExecutionSpaceAnalyzer::create_host_vs_device_summary(stats);
  if (summary.first.kernel_count > 0) {
    const auto& host = summary.first;
    summary_file << "HOST," << host.kernel_count << ","
                 << host.total_duration_ms << "," << std::fixed
                 << std::setprecision(2) << host.average_duration_ms << ","
                 << std::setprecision(6) << host.total_energy_joules << ","
                 << std::setprecision(2) << host.average_power_watts << "\n";
  }
  
  if (summary.second.kernel_count > 0) {
    const auto& device = summary.second;
    summary_file << "DEVICE," << device.kernel_count << ","
                 << device.total_duration_ms << "," << std::fixed
                 << std::setprecision(2) << device.average_duration_ms << ","
                 << std::setprecision(6) << device.total_energy_joules << ","
                 << std::setprecision(2) << device.average_power_watts << "\n";
  }
}

// Output Handler Factory
std::unique_ptr<OutputHandler> OutputHandlerFactory::create(
    HandlerType type, const std::string& file_path) {
  switch (type) {
    case HandlerType::CONSOLE: return std::make_unique<ConsoleOutputHandler>();
    case HandlerType::JSON_FILE:
      if (file_path.empty()) {
        throw std::runtime_error("File path required for JSON output handler");
      }
      return std::make_unique<JsonOutputHandler>(file_path);
    case HandlerType::CSV_FILE:
      if (file_path.empty()) {
        throw std::runtime_error(
            "File path prefix required for CSV output handler");
      }
      return std::make_unique<CsvOutputHandler>(file_path);
    default: throw std::runtime_error("Unknown output handler type");
  }
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

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
      << "timestamp_ms,device_id,device_name,power_watts\n";
  if (readings.empty()) {
    std::cout << "No power readings available\n";
  } else {
    for (const auto& reading : readings) {
      for (const auto& device : reading.device_readings) {
        std::cout << reading.timestamp_ms() << ","
                  << device.device_id << ","
                  << device.device_name << ","
                  << std::fixed << std::setprecision(2)
                  << device.power_watts << '\n';
      }
    }
  }
  std::cout << "--- POWER_DATA_END ---\n";
}

void ConsoleOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings,
    const std::vector<KernelEnergyCorrelation>* correlations) {
  std::cout << "\n--- KERNEL_DATA_START ---\n";
  std::cout << "id,name,type,execution_space,device_id,start_time_ms,end_time_ms,duration_ms";
  if (correlations) {
    std::cout << ",energy_joules,average_power_watts";
  }
  std::cout << "\n";
  
  if (timings.empty()) {
    std::cout << "No kernel data available\n";
  } else {
    for (const auto& timing : timings) {
      std::cout << timing.id << ","
                << "\"" << timing.name << "\","
                << kernel_type_to_string(timing.type) << ","
                << execution_space_to_string(timing.execution_space) << ","
                << timing.device_id << ","
                << timing.start_time_ms() << ","
                << timing.end_time_ms() << ","
                << timing.duration_ms_value();
      
      // Add energy data if available
      if (correlations) {
        bool found = false;
        for (const auto& corr : *correlations) {
          if (corr.kernel_id == timing.id) {
            std::cout << "," << std::fixed << std::setprecision(6)
                      << corr.total_energy_joules << ","
                      << corr.average_power_watts;
            found = true;
            break;
          }
        }
        if (!found) {
          std::cout << ",0.0,0.0";
        }
      }
      std::cout << '\n';
    }
  }
  std::cout << "--- KERNEL_DATA_END ---\n";
}

void ConsoleOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings,
    const std::vector<RegionEnergyCorrelation>* correlations) {
  std::cout << "\n--- REGION_DATA_START ---\n";
  std::cout << "id,name,start_time_ms,end_time_ms,duration_ms";
  if (correlations) {
    std::cout << ",energy_joules,average_power_watts,kernel_ids";
  }
  std::cout << "\n";
  
  if (timings.empty()) {
    std::cout << "No region data available\n";
  } else {
    for (const auto& timing : timings) {
      std::cout << timing.id << ","
                << "\"" << timing.name << "\","
                << timing.start_time_ms() << ","
                << timing.end_time_ms() << ","
                << timing.duration_ms_value();
      
      // Add energy and kernel IDs if available
      if (correlations) {
        bool found = false;
        for (const auto& corr : *correlations) {
          if (corr.region_id == timing.id) {
            std::cout << "," << std::fixed << std::setprecision(6)
                      << corr.total_energy_joules << ","
                      << corr.average_power_watts << ",\"";
            
            // Output comma-separated list of kernel IDs
            bool first = true;
            for (auto kid : corr.kernel_ids) {
              if (!first) std::cout << ";";
              std::cout << kid;
              first = false;
            }
            std::cout << "\"";
            found = true;
            break;
          }
        }
        if (!found) {
          std::cout << ",0.0,0.0,\"\"";
        }
      }
      std::cout << '\n';
    }
  }
  std::cout << "--- REGION_DATA_END ---\n";
}

// JSON Output Handler
JsonOutputHandler::JsonOutputHandler(const std::string& file_path_prefix)
    : file_path_prefix_(file_path_prefix) {}

void JsonOutputHandler::output_power_data(
    const std::vector<EnergyReading>& readings) {
  std::stringstream json;
  json << "{\n";
  json << "  \"power_readings\": [\n";

  if (!readings.empty()) {
    for (size_t i = 0; i < readings.size(); ++i) {
      const auto& reading = readings[i];
      json << "    {\n";
      json << "      \"timestamp_ms\": " << reading.timestamp_ms() << ",\n";
      json << "      \"devices\": [\n";

      size_t device_count = 0;
      for (const auto& device : reading.device_readings) {
        json << "        {\n";
        json << "          \"device_id\": " << device.device_id << ",\n";
        json << "          \"device_name\": \"" << device.device_name << "\",\n";
        json << "          \"power_watts\": " << std::fixed << std::setprecision(2)
            << device.power_watts << "\n";
        json << "        }";
        if (++device_count < reading.device_readings.size()) json << ",";
        json << "\n";
      }

      json << "      ]\n";
      json << "    }";
      if (i < readings.size() - 1) json << ",";
      json << "\n";
    }
  }

  json << "  ]\n";
  json << "}\n";

  write_to_file(json.str(), "_power.json");
}

void JsonOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings,
    const std::vector<KernelEnergyCorrelation>* correlations) {
  std::stringstream json;
  json << "{\n";
  json << "  \"kernel_timings\": [\n";

  if (!timings.empty()) {
    for (size_t i = 0; i < timings.size(); ++i) {
      const auto& timing = timings[i];
      json << "    {\n";
      json << "      \"id\": " << timing.id << ",\n";
      json << "      \"name\": \"" << timing.name << "\",\n";
      json << "      \"type\": \"" << kernel_type_to_string(timing.type) << "\",\n";
      json << "      \"execution_space\": \"" 
           << execution_space_to_string(timing.execution_space) << "\",\n";
      json << "      \"device_id\": " << timing.device_id << ",\n";
      json << "      \"start_time_ms\": " << timing.start_time_ms() << ",\n";
      json << "      \"end_time_ms\": " << timing.end_time_ms() << ",\n";
      json << "      \"duration_ms\": " << timing.duration_ms_value();

      // Add energy data if available
      if (correlations) {
        for (const auto& corr : *correlations) {
          if (corr.kernel_id == timing.id) {
            json << ",\n      \"energy_joules\": " << std::fixed << std::setprecision(6)
                << corr.total_energy_joules << ",\n";
            json << "      \"average_power_watts\": " << corr.average_power_watts;
            break;
          }
        }
      }
      
      json << "\n    }";
      if (i < timings.size() - 1) json << ",";
      json << "\n";
    }
  }

  json << "  ]\n";
  json << "}\n";

  write_to_file(json.str(), "_kernels.json");
}

void JsonOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings,
    const std::vector<RegionEnergyCorrelation>* correlations) {
  std::stringstream json;
  json << "{\n";
  json << "  \"region_timings\": [\n";

  if (!timings.empty()) {
    for (size_t i = 0; i < timings.size(); ++i) {
      const auto& timing = timings[i];
      json << "    {\n";
      json << "      \"id\": " << timing.id << ",\n";
      json << "      \"name\": \"" << timing.name << "\",\n";
      json << "      \"start_time_ms\": " << timing.start_time_ms() << ",\n";
      json << "      \"end_time_ms\": " << timing.end_time_ms() << ",\n";
      json << "      \"duration_ms\": " << timing.duration_ms_value();

      // Add energy data and kernel IDs if available
      if (correlations) {
        for (const auto& corr : *correlations) {
          if (corr.region_id == timing.id) {
            json << ",\n      \"energy_joules\": " << std::fixed << std::setprecision(6)
                << corr.total_energy_joules << ",\n";
            json << "      \"average_power_watts\": " << corr.average_power_watts << ",\n";
            
            // Output array of kernel IDs
            json << "      \"kernel_ids\": [";
            for (size_t k = 0; k < corr.kernel_ids.size(); ++k) {
              json << corr.kernel_ids[k];
              if (k < corr.kernel_ids.size() - 1) json << ", ";
            }
            json << "]";
            break;
          }
        }
      }
      
      json << "\n    }";
      if (i < timings.size() - 1) json << ",";
      json << "\n";
    }
  }

  json << "  ]\n";
  json << "}\n";

  write_to_file(json.str(), "_regions.json");
}

void JsonOutputHandler::write_to_file(const std::string& json_content, const std::string& suffix) {
  std::string filename = file_path_prefix_ + suffix;
  std::ofstream file(filename);
  if (!file) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }
  file << json_content;
  file.close();
}

// CSV Output Handler
CsvOutputHandler::CsvOutputHandler(const std::string& file_path_prefix)
    : file_path_prefix_(file_path_prefix) {}

void CsvOutputHandler::output_power_data(
    const std::vector<EnergyReading>& readings) {
  std::string filename = file_path_prefix_ + "_power.csv";
  std::ofstream file(filename);
  if (!file) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }

  file << "timestamp_ms,device_id,device_name,power_watts\n";
  for (const auto& reading : readings) {
    for (const auto& device : reading.device_readings) {
      file << reading.timestamp_ms() << ","
           << device.device_id << ","
           << "\"" << device.device_name << "\","
           << std::fixed << std::setprecision(2)
           << device.power_watts << '\n';
    }
  }
  file.close();
}

void CsvOutputHandler::output_kernel_data(
    const std::vector<KernelTiming>& timings,
    const std::vector<KernelEnergyCorrelation>* correlations) {
  std::string filename = file_path_prefix_ + "_kernels.csv";
  std::ofstream file(filename);
  if (!file) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }

  file << "id,name,type,execution_space,device_id,start_time_ms,end_time_ms,duration_ms";
  if (correlations) {
    file << ",energy_joules,average_power_watts";
  }
  file << "\n";
  
  for (const auto& timing : timings) {
    file << timing.id << ","
         << "\"" << timing.name << "\","
         << kernel_type_to_string(timing.type) << ","
         << execution_space_to_string(timing.execution_space) << ","
         << timing.device_id << ","
         << timing.start_time_ms() << ","
         << timing.end_time_ms() << ","
         << timing.duration_ms_value();
    
    // Add energy data if available
    if (correlations) {
      bool found = false;
      for (const auto& corr : *correlations) {
        if (corr.kernel_id == timing.id) {
          file << "," << std::fixed << std::setprecision(6)
               << corr.total_energy_joules << ","
               << corr.average_power_watts;
          found = true;
          break;
        }
      }
      if (!found) {
        file << ",0.0,0.0";
      }
    }
    file << '\n';
  }
  file.close();
}

void CsvOutputHandler::output_region_data(
    const std::vector<RegionTiming>& timings,
    const std::vector<RegionEnergyCorrelation>* correlations) {
  std::string filename = file_path_prefix_ + "_regions.csv";
  std::ofstream file(filename);
  if (!file) {
    throw std::runtime_error("Failed to open output file: " + filename);
  }

  file << "id,name,start_time_ms,end_time_ms,duration_ms";
  if (correlations) {
    file << ",energy_joules,average_power_watts,kernel_ids";
  }
  file << "\n";
  
  for (const auto& timing : timings) {
    file << timing.id << ","
         << "\"" << timing.name << "\","
         << timing.start_time_ms() << ","
         << timing.end_time_ms() << ","
         << timing.duration_ms_value();
    
    // Add energy and kernel IDs if available
    if (correlations) {
      bool found = false;
      for (const auto& corr : *correlations) {
        if (corr.region_id == timing.id) {
          file << "," << std::fixed << std::setprecision(6)
               << corr.total_energy_joules << ","
               << corr.average_power_watts << ",\"";
          
          // Output semicolon-separated list of kernel IDs
          bool first = true;
          for (auto kid : corr.kernel_ids) {
            if (!first) file << ";";
            file << kid;
            first = false;
          }
          file << "\"";
          found = true;
          break;
        }
      }
      if (!found) {
        file << ",0.0,0.0,\"\"";
      }
    }
    file << '\n';
  }
  file.close();
}

// Factory implementation
std::unique_ptr<OutputHandler> OutputHandlerFactory::create(
    OutputHandlerFactory::Format format, const std::string& file_path_prefix) {
  switch (format) {
    case Format::CONSOLE:
      return std::make_unique<ConsoleOutputHandler>();
    case Format::JSON:
      return std::make_unique<JsonOutputHandler>(file_path_prefix);
    case Format::CSV:
      return std::make_unique<CsvOutputHandler>(file_path_prefix);
  }
  // Should not reach here, but fallback to console output
  return std::make_unique<ConsoleOutputHandler>();
}

}  // namespace PowerProfiler
}  // namespace KokkosTools

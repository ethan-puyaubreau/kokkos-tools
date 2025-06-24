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

#include "../data/energy_metrics.hpp"
#include "../data/timing_data.hpp"
#include "../data/correlation.hpp"
#include <vector>
#include <string>
#include <memory>

namespace KokkosTools {
namespace PowerProfiler {

// Forward declaration
class OutputFormat;

class OutputHandler {
 public:
  virtual ~OutputHandler() = default;

  // Raw data output methods
  virtual void output_power_data(
      const std::vector<EnergyReading>& readings) = 0;
  virtual void output_kernel_data(
      const std::vector<KernelTiming>& timings, 
      const std::vector<KernelEnergyCorrelation>* correlations = nullptr) = 0;
  virtual void output_region_data(
      const std::vector<RegionTiming>& timings,
      const std::vector<RegionEnergyCorrelation>* correlations = nullptr) = 0;
};

class ConsoleOutputHandler : public OutputHandler {
 public:
  void output_power_data(const std::vector<EnergyReading>& readings) override;
  void output_kernel_data(
      const std::vector<KernelTiming>& timings, 
      const std::vector<KernelEnergyCorrelation>* correlations = nullptr) override;
  void output_region_data(
      const std::vector<RegionTiming>& timings,
      const std::vector<RegionEnergyCorrelation>* correlations = nullptr) override;
};

class JsonOutputHandler : public OutputHandler {
 public:
  explicit JsonOutputHandler(const std::string& file_path_prefix);

  void output_power_data(const std::vector<EnergyReading>& readings) override;
  void output_kernel_data(
      const std::vector<KernelTiming>& timings, 
      const std::vector<KernelEnergyCorrelation>* correlations = nullptr) override;
  void output_region_data(
      const std::vector<RegionTiming>& timings,
      const std::vector<RegionEnergyCorrelation>* correlations = nullptr) override;

 private:
  void write_to_file(const std::string& json_content, const std::string& suffix);
  std::string file_path_prefix_;
};

class CsvOutputHandler : public OutputHandler {
 public:
  explicit CsvOutputHandler(const std::string& file_path_prefix);

  void output_power_data(const std::vector<EnergyReading>& readings) override;
  void output_kernel_data(
      const std::vector<KernelTiming>& timings, 
      const std::vector<KernelEnergyCorrelation>* correlations = nullptr) override;
  void output_region_data(
      const std::vector<RegionTiming>& timings,
      const std::vector<RegionEnergyCorrelation>* correlations = nullptr) override;

 private:
  std::string file_path_prefix_;
};

// Factory for output handlers
class OutputHandlerFactory {
 public:
  enum class Format { CONSOLE, JSON, CSV };
  
  static std::unique_ptr<OutputHandler> create(Format format, 
                                               const std::string& file_path_prefix = "");
};

}  // namespace PowerProfiler
}  // namespace KokkosTools

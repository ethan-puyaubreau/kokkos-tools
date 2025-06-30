# NVML Energy and Power Profilers

This directory contains two complementary NVML-based profilers for monitoring GPU energy consumption and power usage during Kokkos application execution.

## Available Profilers

### 1. `kp_energy_nvml` - Direct Energy Measurement
- **Method**: Uses `nvmlDeviceGetTotalEnergyConsumption()` to read accumulated energy counters
- **Approach**: Takes energy readings at application start and end, calculates difference
- **Strengths**: 
  - Potentially more accurate for total energy consumption
  - Hardware-based energy accumulation counters
  - Less susceptible to sampling errors
- **Output Files**: 
  - `hostname-pid-nvml-energy.csv` (absolute timestamps + energy)
  - `hostname-pid-nvml-energy-relative.csv` (relative time + energy)
  - `hostname-pid-nvml-energy.dat` (summary statistics)

### 2. `kp_power_nvml` - Power Integration Measurement  
- **Method**: Uses `nvmlDeviceGetPowerUsage()` to sample instantaneous power consumption
- **Approach**: Polls power every 100ms, integrates over time using trapezoidal rule
- **Strengths**:
  - Detailed temporal power profile information
  - Visibility into power variations during execution
  - Additional statistics (min/max/average power)
- **Output Files**:
  - `hostname-pid-nvml-power.csv` (timestamps + power + integrated energy)
  - `hostname-pid-nvml-power-relative.csv` (relative data)
  - `hostname-pid-nvml-power.dat` (comprehensive statistics)

## Measurement Methodology Differences

The two profilers may report different energy values for the same workload due to their fundamentally different measurement approaches:

### Energy Counter vs Power Integration
- **Energy Counter Approach** (`kp_energy_nvml`): Reads hardware-maintained cumulative energy registers, which could potentially capture all energy consumption including rapid transients
- **Power Integration Approach** (`kp_power_nvml`): Samples instantaneous power and numerically integrates, which might miss very short power spikes between 100ms sampling intervals

### Potential Sources of Measurement Differences

1. **Sampling Resolution**: Power integration method samples every 100ms, potentially missing brief high-power events that would be captured by cumulative energy counters

2. **Hardware Sensor Differences**: The underlying NVML functions may use different sensor systems or aggregation methods within the GPU

3. **Measurement Timing**: 
   - Energy counters provide end-to-end consumption for the entire measurement period
   - Power integration begins from first sample, potentially missing initialization power spikes

4. **Baseline Power Handling**: Power integration includes all sampled values, including idle/low-power states during application startup, which could affect the integrated total

## Usage Recommendations

### For Total Energy Analysis
Use `kp_energy_nvml` when you need:
- Most accurate total energy consumption figures
- Simple energy budget calculations
- Minimal measurement overhead
- Hardware-validated energy totals

### For Power Profile Analysis  
Use `kp_power_nvml` when you need:
- Detailed power consumption timeline
- Power variation statistics (min/max/average)
- Temporal correlation with application phases
- Power spike detection and analysis

## Building and Usage

Both profilers are built automatically when NVML is available:

```bash
# Build both profilers
make -j

# Use energy profiler
export KOKKOS_TOOLS_LIBS=/path/to/kp_energy_nvml.so
./your_kokkos_application

# Use power profiler  
export KOKKOS_TOOLS_LIBS=/path/to/kp_power_nvml.so
./your_kokkos_application
```

NVML can also be manually specified in your CMake configuration if needed:

```bash
cmake -DNVML_ROOT=/path/to/nvidia/nvml ..
make -j
```

## Requirements

- NVIDIA GPU with NVML support
- NVIDIA drivers installed
- CUDA toolkit (for NVML headers and libraries)
- C++20 compatible compiler

## Validation Considerations

When comparing results between profilers, consider that:
- Values may differ due to measurement methodology, not measurement error
- Both approaches provide valid but potentially different perspectives on GPU energy consumption
- Cross-validation with external power measurement tools could help establish ground truth
- The "most accurate" method may depend on specific use case and GPU workload characteristics

For critical energy analysis applications, consider running both profilers and analyzing the differences to understand the energy consumption profile of your specific workload.
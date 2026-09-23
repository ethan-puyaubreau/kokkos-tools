# Kokkos Tools Energy Profiler (V1 Specification)

This profiling connector hooks into the `KokkosP` instrumentation callbacks to capture kernel/region timings and sample GPU physical power usage in real time.

## Data Output Format

The connector outputs three files to the directory specified by `KOKKOS_TOOLS_OUTPUT_PATH` (default: current directory):

1. `events.csv`: Nanosecond execution bounds of Kokkos regions and parallel dispatches.
   ```csv
   id,parent_id,name,category,start_ns,end_ns
   ```
2. `power_samples.csv`: High-frequency power samples (50 Hz default) captured asynchronously via NVIDIA NVML.
   ```csv
   timestamp_ns,domain,device_id,power_watts,energy_joules
   ```
3. `metadata.json`: Host and backend execution metadata.

## Building

```bash
g++ -std=c++20 -O3 -fPIC -shared kp_energy_profiler.cpp \
    -I/usr/local/cuda/include \
    -L/usr/lib/x86_64-linux-gnu -lnvidia-ml -lpthread \
    -o libkokkos_energy.so
```

## Usage

```bash
export KOKKOS_TOOLS_LIBS=/path/to/libkokkos_energy.so
export KOKKOS_TOOLS_OUTPUT_PATH=./my_trace
./my_kokkos_app
```

The resulting trace directory can then be analyzed using `kokkos-energy`.

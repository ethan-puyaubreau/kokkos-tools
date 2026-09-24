# Kokkos Tools Energy Profiler (V1 Specification)

This profiling connector hooks into the `KokkosP` instrumentation callbacks to capture kernel/region timings and sample NVIDIA GPU power usage in real time.

This directory is the source of truth for the connector. The trace format it writes is
specified in the
[DATA_SPEC.md](https://github.com/ethan-puyaubreau/energy-dashboard-for-kokkos/blob/main/DATA_SPEC.md)
of energy-dashboard-for-kokkos, which reads these traces.

## Data Output Format

The connector outputs three files to the directory specified by `KOKKOS_TOOLS_OUTPUT_PATH` (default: current directory),
in a `rank_<N>` subdirectory when an MPI rank is detected:

1. `events.csv`: Nanosecond execution bounds of Kokkos regions, parallel dispatches and deep copies.
   ```csv
   id,parent_id,name,category,start_ns,end_ns
   ```
2. `power_samples.csv`: Power samples of every NVIDIA GPU (50 Hz), captured asynchronously via NVML.
   ```csv
   timestamp_ns,domain,device_id,power_watts
   ```
3. `metadata.json`: Host and backend execution metadata (`spec_version`, `app_name`, `hostname`,
   `kokkos_backend`, `device_count`, `start_epoch_ns`, and `mpi_rank` under MPI).

## Building

Requires CMake 3.18 or newer and the CUDA toolkit, which provides NVML.

```bash
cmake -S profiling/energy-profiler -B build-energy -DCMAKE_BUILD_TYPE=Release
cmake --build build-energy
# -> build-energy/libkp_energy_profiler.so
```

Without a GPU, `-DKP_ENERGY_PROFILER_NVML_STUB=ON` builds against an NVML stub reporting one
GPU at 100 W, together with `kp_energy_profiler_smoke`, a driver that loads the connector
like Kokkos Tools does and records one region around one kernel:

```bash
cmake -S profiling/energy-profiler -B build-energy -DKP_ENERGY_PROFILER_NVML_STUB=ON
cmake --build build-energy
KOKKOS_TOOLS_OUTPUT_PATH=trace build-energy/kp_energy_profiler_smoke build-energy/libkp_energy_profiler.so
```

## Usage

```bash
export KOKKOS_TOOLS_LIBS=/path/to/libkp_energy_profiler.so
export KOKKOS_TOOLS_OUTPUT_PATH=./my_trace
./my_kokkos_app
```

The resulting trace directory can then be analyzed with
[energy-dashboard-for-kokkos](https://github.com/ethan-puyaubreau/energy-dashboard-for-kokkos).

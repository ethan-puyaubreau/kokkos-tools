# Variorum Energy Profiler for Kokkos Tools

This module provides an energy profiling tool for Kokkos applications, based on the [Variorum](https://github.com/LLNL/variorum) library. It allows measuring the energy consumption of different hardware components (CPU, GPU, etc.) during the execution of Kokkos kernels.

## Features

- **Energy monitoring**: Periodic measurement of power consumed by supported devices via Variorum.
- **Kernel-level profiling**: Association of energy measurements with executed Kokkos kernels.
- **Configurable output**: Ability to set the measurement interval and output path through environment variables.

## Environment Variables

- `KOKKOS_TOOLS_POWER_MONITOR_INTERVAL`: Measurement interval in microseconds (us). Default: 20,000 us (20 ms).
- `KOKKOS_TOOLS_POWER_OUTPUT_PATH`: Output file path for profiling results.

## Compilation

This module requires the Variorum library to be installed on the system.

### With CMake

```sh
cmake -B build
cmake --build build
```

### With Makefile

Set the `VARIORUM_ROOT` environment variable pointing to the Variorum installation, then:

```sh
make
```

## Usage

1. Compile the module to generate the shared library `kp_energy_variorum.so`.
2. Load this module as a Kokkos profiling tool using the environment variable:
    ```sh
    export KOKKOS_PROFILE_LIBRARY=/path/to/kp_energy_variorum.so
    ```
3. Run your Kokkos application normally.

## Dependencies

- [Variorum](https://github.com/LLNL/variorum)
- [jansson](https://digip.org/jansson/) (for JSON parsing)

## References

- [Variorum Documentation](https://github.com/LLNL/variorum)

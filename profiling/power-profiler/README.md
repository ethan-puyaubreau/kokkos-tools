This Kokkos power profiler is a **modular tool** that integrates with Kokkos to track **energy consumption** and **kernel execution times** (parallel\_for, parallel\_scan, parallel\_reduce). It's designed for flexibility with interchangeable energy providers and offers detailed analysis of energy-performance correlation.

The tool connects to Kokkos's profiling interface. When your application starts, it initializes an energy provider (like Variorum for real hardware measurements or a "dummy" provider for development), detects devices, and launches a **dedicated monitoring thread** that continuously samples power. During execution, it captures Kokkos kernel events and their timings. At the end, it correlates energy and performance data and generates reports.

-----

### Quick Usage

1.  **Prerequisites:**

      * Kokkos with the profiling interface enabled.
      * A C++17 compiler.
      * For hardware monitoring: **Variorum** library installed.

2.  **Building (CMake Recommended):**

    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --target kp_power_profiler
    # The library will be in build/profiling/power-profiler/libkp_power_profiler.so
    ```

3.  **Configuration via Environment Variables:**

    Set these variables **before** launching your application:

      * `KOKKOS_POWER_PROFILER_PROVIDER`: Choose `VARIORUM` (for real data) or `DUMMY` (for testing without hardware).
      * `KOKKOS_POWER_PROFILER_INTERVAL`: Sampling interval in microseconds (e.g., `20000` for 20ms).
      * `KOKKOS_POWER_PROFILER_OUTPUT`: Output format (`CONSOLE`, `JSON`, `CSV`).
      * `KOKKOS_POWER_PROFILER_OUTPUT_FILE`: Output file name (if JSON/CSV).

    **Example:**

    ```bash
    export KOKKOS_POWER_PROFILER_PROVIDER=VARIORUM
    export KOKKOS_POWER_PROFILER_INTERVAL=20000
    export KOKKOS_POWER_PROFILER_OUTPUT=JSON
    export KOKKOS_POWER_PROFILER_OUTPUT_FILE=my_energy_profile
    ```

4.  **Running Your Application:**

    Set the path to the profiler library:

    ```bash
    export KOKKOS_TOOLS_LIBS=/path/to/libkp_power_profiler.so
    ./your_kokkos_application
    ```

    Your results will be generated according to the specified format and file (e.g., `my_energy_profile.json`).

-----

### Outputs

The profiler generates data on **energy readings** (timestamp, device ID, power in Watts) and **kernel timings** (type, name, start, end, duration in nanoseconds).
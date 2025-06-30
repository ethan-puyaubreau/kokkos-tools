# Kokkos Tool - Energy Profiling

This folder contains a series of work in progress Kokkos Tools dedicated to energy profiling within Kokkos programs.

Due to the diversity of data sources and means of energy profiling, many different tools are developed separately in order to study the possibilities of measuring energy consumption software wise.

Here is the current list of tools (WIP) :
---
- Variorum energy profiler
    - NVIDIA, AMD, Intel compatibility.
- PAPI energy profiler
    - NVIDIA, AMD, Intel compatibility.
- NVML energy profiler
    - NVIDIA GPU Only
- RAPL energy profiler
    - Intel/AMD CPU only
- ROCM energy profiler
    - AMD GPU only

The data extracted from each tool and its structure is specified in the tool folder.
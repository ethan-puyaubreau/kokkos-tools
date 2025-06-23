kp_add_library(kp_power_profiler 
    src/power-profiler.cpp
    src/core/daemon.cpp
    src/core/timing_manager.cpp
    src/core/config.cpp
    src/core/power_monitor.cpp
    src/core/profiler_core.cpp
    src/providers/energy_provider.cpp
    src/providers/variorum_energy_provider.cpp
    src/providers/dummy_energy_provider.cpp
    src/data/timing_data.cpp
    src/data/correlation.cpp
    src/output/output_handler.cpp
)

# Add include directories for the new structure
target_include_directories(kp_power_profiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/providers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/output
)

# Set C++17 standard (minimum required for steady_clock and other features)
target_compile_features(kp_power_profiler PRIVATE cxx_std_17)

target_link_libraries(kp_power_profiler PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_power_profiler PRIVATE MPI::MPI_CXX)
endif()

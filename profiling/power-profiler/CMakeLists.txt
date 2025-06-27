kp_add_library(kp_power_profiler 
    power-profiler.cpp
    variorum_power_profiler.cpp
)

target_include_directories(kp_power_profiler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(CMAKE_CXX_STANDARD 20)

target_link_libraries(kp_power_profiler PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_power_profiler PRIVATE MPI::MPI_CXX)
endif()

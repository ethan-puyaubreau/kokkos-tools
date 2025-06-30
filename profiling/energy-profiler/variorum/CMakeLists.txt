set(CMAKE_CXX_STANDARD 17)

kp_add_library(kp_energy_variorum kp_energy_variorum.cpp
    variorum_energy_profiler.cpp
)

target_include_directories(kp_energy_variorum PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(kp_energy_variorum PRIVATE variorum::variorum)

if(USE_MPI)
  target_link_libraries(kp_energy_variorum PRIVATE MPI::MPI_CXX)
endif()
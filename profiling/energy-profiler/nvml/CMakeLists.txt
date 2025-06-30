find_package(CUDAToolkit QUIET)

# Find NVML
if(CUDAToolkit_FOUND)
    set(NVML_INCLUDE_DIR "${CUDAToolkit_INCLUDE_DIRS}")
    
    find_library(NVML_LIBRARY 
        NAMES nvidia-ml nvml
        PATHS "${CUDAToolkit_LIBRARY_DIR}" "/usr/lib64" "/usr/lib/x86_64-linux-gnu" 
        PATH_SUFFIXES "stubs"
        DOC "NVIDIA Management Library")
else()
    find_path(NVML_INCLUDE_DIR
        NAMES nvml.h
        PATHS ENV CUDA_PATH ENV CUDA_HOME ENV CUDA_TOOLKIT_ROOT_DIR
                    /usr/local/cuda /opt/cuda /usr
        PATH_SUFFIXES include
        DOC "NVML header location")
        
    find_library(NVML_LIBRARY
        NAMES nvidia-ml nvml
        PATHS ENV CUDA_PATH ENV CUDA_HOME ENV LD_LIBRARY_PATH
                    /usr/lib64 /usr/lib/x86_64-linux-gnu /usr/local/cuda/lib64
        DOC "NVIDIA Management Library")
endif()

if(NVML_INCLUDE_DIR AND NVML_LIBRARY AND EXISTS "${NVML_INCLUDE_DIR}/nvml.h")
    message(STATUS "Found NVML, building NVML-based tools.")
    message(STATUS "NVML include dir: ${NVML_INCLUDE_DIR}")
    message(STATUS "NVML library: ${NVML_LIBRARY}")

    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    kp_add_library(kp_energy_nvml kp_energy_nvml.cpp)

    target_include_directories(kp_energy_nvml PRIVATE 
        ${NVML_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(kp_energy_nvml 
        ${NVML_LIBRARY}
        pthread
    )

    get_filename_component(NVML_LIBRARY_DIR "${NVML_LIBRARY}" DIRECTORY)
    set_target_properties(kp_energy_nvml PROPERTIES
        BUILD_RPATH "${NVML_LIBRARY_DIR}"
        INSTALL_RPATH "${NVML_LIBRARY_DIR}"
    )

    kp_add_library(kp_power_nvml kp_power_nvml.cpp)

    target_include_directories(kp_power_nvml PRIVATE 
        ${NVML_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    target_link_libraries(kp_power_nvml 
        ${NVML_LIBRARY}
        pthread
    )

    set_target_properties(kp_power_nvml PROPERTIES
        BUILD_RPATH "${NVML_LIBRARY_DIR}"
        INSTALL_RPATH "${NVML_LIBRARY_DIR}"
    )
else()
    message(WARNING "NVML not found, NVML-based tools will not be built. To build them, ensure CUDA is installed and environment variables like CUDA_PATH are set.")
endif()

## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_macros)
include(openvkl_dpcpp)

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/openvkl/include
  ${RKCOMMON_INCLUDE_DIRS}
)

# directory of device implementations; we avoid relative paths since some CMake
# macros have difficult with such paths.
set(DEVICE_ROOT_DIR ${PROJECT_SOURCE_DIR}/openvkl/devices)

# sources common to multiple volume types
set(SOURCES_COMMON_AMR_PARTICLE_UNSTRUCTURED
  ${DEVICE_ROOT_DIR}/cpu/iterator/UnstructuredIterator.cpp
  ${DEVICE_ROOT_DIR}/cpu/iterator/UnstructuredIterator.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/UnstructuredVolume.ispc
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_VDB
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbVolume.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbVolume.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbSampler.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbSampler.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbInnerNodeObserver.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbIterator.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbIterator.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/vdb/VdbLeafAccessObserver.cpp
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_LEGACY_SPHERICAL
  ${DEVICE_ROOT_DIR}/cpu/iterator/GridAcceleratorIterator.cpp
  ${DEVICE_ROOT_DIR}/cpu/iterator/GridAcceleratorIterator.ispc
  ${DEVICE_ROOT_DIR}/cpu/iterator/GridAcceleratorIteratorSize.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/GridAccelerator.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/SharedStructuredVolume.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/StructuredVolume.cpp
)

# source files per volume type
set(SOURCES_STRUCTURED_REGULAR_LEGACY
  ${DEVICE_ROOT_DIR}/cpu/volume/StructuredRegularVolume.cpp
)

set(SOURCES_STRUCTURED_SPHERICAL
  ${DEVICE_ROOT_DIR}/cpu/volume/StructuredSphericalVolume.cpp
)

set(SOURCES_UNSTRUCTURED
  ${DEVICE_ROOT_DIR}/cpu/volume/UnstructuredVolume.cpp
)

set(SOURCES_PARTICLE
  ${DEVICE_ROOT_DIR}/cpu/volume/particle/ParticleVolume.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/particle/ParticleVolume.ispc
)

set(SOURCES_AMR
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/AMRAccel.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/AMRData.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/AMRVolume.cpp
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/AMRVolume.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/CellRef.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/DualCell.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/method_current.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/method_finest.ispc
  ${DEVICE_ROOT_DIR}/cpu/volume/amr/method_octant.ispc
)

# generate source file additions for optional volume types
list(APPEND OPTIONAL_VOLUME_SOURCES)
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "")

# TODO: continue to use "CPU" naming for a while, as using CPUDevice code...
list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_STRUCTURED_REGULAR_LEGACY})
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY=1")

list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_STRUCTURED_SPHERICAL})
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL=1")

list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_UNSTRUCTURED})
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_UNSTRUCTURED=1")

list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_PARTICLE})
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_PARTICLE=1")

# VDB volume type uses common sources only: SOURCES_COMMON_STRUCTURED_REGULAR_VDB
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_VDB=1")

list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_AMR})
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_AMR=1")

# common source files
list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_AMR_PARTICLE_UNSTRUCTURED})
list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_STRUCTURED_REGULAR_VDB})
list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_STRUCTURED_REGULAR_LEGACY_SPHERICAL})

# definitions for ISPC components
set(OPTIONAL_VOLUME_DEFINITIONS_ISPC ${OPTIONAL_VOLUME_DEFINITIONS})
list(TRANSFORM OPTIONAL_VOLUME_DEFINITIONS_ISPC PREPEND "-D")

# build only a single ISPC target (for width 4) for now
set(TARGET_WIDTH 4)

set(ISPC_TARGETS_OVERRIDE sse4 sse2) # multiple targets MUST be enabled due to ISPC name mangling issues

set(ISPC_DEFINITIONS
  "-DVKL_TARGET_WIDTH=${TARGET_WIDTH}"
  ${OPTIONAL_VOLUME_DEFINITIONS_ISPC}
)

openvkl_add_library_ispc(openvkl_module_gpu_device STATIC
  api/DeviceAPI.cpp
  api/GPUDevice.cpp
  ${DEVICE_ROOT_DIR}/cpu/iterator/DefaultIterator.cpp
  ${DEVICE_ROOT_DIR}/cpu/iterator/DefaultIterator.ispc
  ${DEVICE_ROOT_DIR}/cpu/iterator/IteratorContext.cpp
  ${DEVICE_ROOT_DIR}/cpu/iterator/IteratorContext.ispc
  ${DEVICE_ROOT_DIR}/cpu/observer/Observer.cpp
  ${DEVICE_ROOT_DIR}/cpu/observer/ObserverRegistry.cpp
  ${DEVICE_ROOT_DIR}/cpu/observer/ObserverRegistry.ispc
  ${DEVICE_ROOT_DIR}/cpu/sampler/Sampler.cpp
  ${DEVICE_ROOT_DIR}/cpu/sampler/Sampler.ispc
  ${OPTIONAL_VOLUME_SOURCES}
  ${VKL_RESOURCE}
)

unset(ISPC_DEFINITIONS)
unset(ISPC_TARGETS_OVERRIDE)

target_compile_definitions(openvkl_module_gpu_device PUBLIC
  "VKL_TARGET_WIDTH=${TARGET_WIDTH}"
  ${OPTIONAL_VOLUME_DEFINITIONS}
)

set(width_compile_options "")
openvkl_get_compile_options_for_width(${TARGET_WIDTH} width_compile_options)
separate_arguments(width_compile_options UNIX_COMMAND "${width_compile_options}")

message(STATUS "using compile options for width ${TARGET_WIDTH}: ${width_compile_options}")

target_compile_options(openvkl_module_gpu_device PRIVATE ${width_compile_options})

openvkl_add_sycl_target(openvkl_module_gpu_device)

target_include_directories(openvkl_module_gpu_device
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>
)

target_link_libraries(openvkl_module_gpu_device
  PUBLIC
  openvkl
  $<BUILD_INTERFACE:rkcommon::rkcommon>
  $<BUILD_INTERFACE:embree>
)

target_compile_definitions(openvkl_module_gpu_device PUBLIC
  "VKL_TARGET_WIDTH_ENABLED_${TARGET_WIDTH}")

target_include_directories(openvkl_module_gpu_device INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/openvkl/devices/gpu>
)

target_include_directories(openvkl_module_gpu_device
  INTERFACE $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>)

openvkl_install_library(openvkl_module_gpu_device)

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openvkl
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openvkl/devices/gpu
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.isph"
)

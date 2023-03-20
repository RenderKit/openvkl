## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_macros)
include(openvkl_dpcpp)

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/openvkl/include
  ${RKCOMMON_INCLUDE_DIRS}
)

# sources common to multiple volume types
set(SOURCES_COMMON_AMR_PARTICLE_UNSTRUCTURED
  ../cpu/iterator/UnstructuredIterator.cpp
  ../cpu/iterator/UnstructuredIterator.ispc
  ../cpu/volume/UnstructuredVolume.ispc
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_VDB
  ../cpu/volume/vdb/VdbVolume.cpp
  ../cpu/volume/vdb/VdbVolume.ispc
  ../cpu/volume/vdb/VdbSampler.cpp
  ../cpu/volume/vdb/VdbSampler.ispc
  ../cpu/volume/vdb/VdbInnerNodeObserver.cpp
  ../cpu/volume/vdb/VdbIterator.cpp
  ../cpu/volume/vdb/VdbIterator.ispc
  ../cpu/volume/vdb/VdbLeafAccessObserver.cpp
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_LEGACY_SPHERICAL
  ../cpu/iterator/GridAcceleratorIterator.cpp
  ../cpu/iterator/GridAcceleratorIterator.ispc
  ../cpu/iterator/GridAcceleratorIteratorSize.ispc
  ../cpu/volume/GridAccelerator.ispc
  ../cpu/volume/SharedStructuredVolume.ispc
  ../cpu/volume/StructuredVolume.cpp
)

# source files per volume type
set(SOURCES_STRUCTURED_REGULAR_LEGACY
  ../cpu/volume/StructuredRegularVolume.cpp
)

set(SOURCES_STRUCTURED_SPHERICAL
  ../cpu/volume/StructuredSphericalVolume.cpp
)

set(SOURCES_UNSTRUCTURED
  ../cpu/volume/UnstructuredVolume.cpp
)

set(SOURCES_PARTICLE
  ../cpu/volume/particle/ParticleVolume.cpp
  ../cpu/volume/particle/ParticleVolume.ispc
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
  ../cpu/iterator/DefaultIterator.cpp
  ../cpu/iterator/DefaultIterator.ispc
  ../cpu/iterator/IteratorContext.cpp
  ../cpu/iterator/IteratorContext.ispc
  ../cpu/observer/Observer.cpp
  ../cpu/observer/ObserverRegistry.cpp
  ../cpu/observer/ObserverRegistry.ispc
  ../cpu/sampler/Sampler.cpp
  ../cpu/sampler/Sampler.ispc
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
    $<BUILD_INTERFACE:${ISPCRT_INCLUDE_DIR}>
)

target_link_libraries(openvkl_module_gpu_device
  PUBLIC
    openvkl
    $<BUILD_INTERFACE:rkcommon::rkcommon>
    $<BUILD_INTERFACE:ispcrt::ispcrt>
  PRIVATE
    embree
    $<INSTALL_INTERFACE:rkcommon::rkcommon>
)

target_compile_definitions(openvkl_module_gpu_device PUBLIC
  "VKL_TARGET_WIDTH_ENABLED_${TARGET_WIDTH}")

target_include_directories(openvkl_module_gpu_device INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include/openvkl/devices/gpu>
)

target_include_directories(openvkl_module_gpu_device
  INTERFACE $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>)

target_link_libraries(openvkl_module_gpu_device
  PUBLIC
    openvkl
)

openvkl_install_library(openvkl_module_gpu_device)

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/include/openvkl
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/openvkl/devices/gpu
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.isph"
)

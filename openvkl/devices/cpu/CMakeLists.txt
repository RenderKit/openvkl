## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_ispc)
include(openvkl_macros)

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/openvkl/include
  ${RKCOMMON_INCLUDE_DIRS}
)

option(VKL_BUILD_VDB_ITERATOR_SIZE_HELPER "Build helper program that computes sizeof(VdbIterator)" OFF)

# for toggling enablement of specific volume types
option(OPENVKL_DEVICE_CPU_AMR "enable CPU device AMR volume type" ON)
option(OPENVKL_DEVICE_CPU_PARTICLE "enable CPU device particle volume type" ON)
option(OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR "enable CPU device structuredRegular volume type" ON)
option(OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY "enable CPU device structuredRegular (legacy) volume type" OFF)
option(OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL "enable CPU device structuredSpherical volume type" ON)
option(OPENVKL_DEVICE_CPU_UNSTRUCTURED "enable CPU device unstructured volume type" ON)
option(OPENVKL_DEVICE_CPU_VDB "enable CPU device VDB volume type" ON)

# these options are not documented, marked as advanced, and may change in the future
mark_as_advanced(
  OPENVKL_DEVICE_CPU_AMR
  OPENVKL_DEVICE_CPU_PARTICLE
  OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
  OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY
  OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL
  OPENVKL_DEVICE_CPU_UNSTRUCTURED
  OPENVKL_DEVICE_CPU_VDB
)

if(${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR} AND ${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY})
  message(FATAL_ERROR "cannot enable both OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR and OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY")
endif()

# sources common to multiple volume types
set(SOURCES_COMMON_AMR_PARTICLE_UNSTRUCTURED
  iterator/UnstructuredIterator.cpp
  iterator/UnstructuredIterator.ispc
  volume/UnstructuredVolume.ispc
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_VDB
  volume/vdb/VdbVolume.cpp
  volume/vdb/VdbVolume.ispc
  volume/vdb/VdbSampler.cpp
  volume/vdb/VdbSampler.ispc
  volume/vdb/VdbInnerNodeObserver.cpp
  volume/vdb/VdbIterator.cpp
  volume/vdb/VdbIterator.ispc
  volume/vdb/VdbLeafAccessObserver.cpp
)

set(SOURCES_COMMON_STRUCTURED_REGULAR_LEGACY_SPHERICAL
  iterator/GridAcceleratorIterator.cpp
  iterator/GridAcceleratorIterator.ispc
  iterator/GridAcceleratorIteratorSize.ispc
  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/StructuredVolume.cpp
)

# source files per volume type
set(SOURCES_AMR
  volume/amr/AMRAccel.cpp
  volume/amr/AMRData.cpp
  volume/amr/AMRVolume.cpp
  volume/amr/AMRVolume.ispc
  volume/amr/CellRef.ispc
  volume/amr/DualCell.ispc
  volume/amr/method_current.ispc
  volume/amr/method_finest.ispc
  volume/amr/method_octant.ispc
)

set(SOURCES_PARTICLE
  volume/particle/ParticleVolume.cpp
  volume/particle/ParticleVolume.ispc
)

set(SOURCES_STRUCTURED_REGULAR
  volume/vdb/DenseVdbVolume.cpp
)

set(SOURCES_STRUCTURED_REGULAR_LEGACY
  volume/StructuredRegularVolume.cpp
)

set(SOURCES_STRUCTURED_SPHERICAL
  volume/StructuredSphericalVolume.cpp
)

set(SOURCES_UNSTRUCTURED
  volume/UnstructuredVolume.cpp
)

# generate source file additions for optional volume types
list(APPEND OPTIONAL_VOLUME_SOURCES)
list(APPEND OPTIONAL_VOLUME_DEFINITIONS "")

if(${OPENVKL_DEVICE_CPU_AMR})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_AMR})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_AMR=1")
endif()

if(${OPENVKL_DEVICE_CPU_PARTICLE})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_PARTICLE})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_PARTICLE=1")
endif()

if(${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_STRUCTURED_REGULAR})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR=1")
endif()

if(${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_STRUCTURED_REGULAR_LEGACY})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY=1")
endif()

if(${OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_STRUCTURED_SPHERICAL})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL=1")
endif()

if(${OPENVKL_DEVICE_CPU_UNSTRUCTURED})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_UNSTRUCTURED})
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_UNSTRUCTURED=1")
endif()

if(${OPENVKL_DEVICE_CPU_VDB})
  # uses common sources only: SOURCES_COMMON_STRUCTURED_REGULAR_VDB
  list(APPEND OPTIONAL_VOLUME_DEFINITIONS "OPENVKL_DEVICE_CPU_VDB=1")
endif()

# common source files
if(${OPENVKL_DEVICE_CPU_AMR} OR
   ${OPENVKL_DEVICE_CPU_PARTICLE} OR
   ${OPENVKL_DEVICE_CPU_UNSTRUCTURED})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_AMR_PARTICLE_UNSTRUCTURED})
endif()

if(${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR} OR
   ${OPENVKL_DEVICE_CPU_VDB})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_STRUCTURED_REGULAR_VDB})
endif()

if(${OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY} OR
   ${OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL})
  list(APPEND OPTIONAL_VOLUME_SOURCES ${SOURCES_COMMON_STRUCTURED_REGULAR_LEGACY_SPHERICAL})
endif()

# definitions for ISPC components
set(OPTIONAL_VOLUME_DEFINITIONS_ISPC ${OPTIONAL_VOLUME_DEFINITIONS})
list(TRANSFORM OPTIONAL_VOLUME_DEFINITIONS_ISPC PREPEND "-D")

# width-specific builds
foreach(TARGET_WIDTH 4 8 16)

  openvkl_get_width_enabled(${TARGET_WIDTH} TARGET_WIDTH_ENABLED)

  set(TARGET_WIDTH_ENABLED_${TARGET_WIDTH} ${TARGET_WIDTH_ENABLED})

  if(NOT ${TARGET_WIDTH_ENABLED})
    continue()
  endif()

  set(TARGET_NAME "openvkl_module_cpu_device_${TARGET_WIDTH}")

  if (TARGET_WIDTH EQUAL 4)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_4})
  elseif (TARGET_WIDTH EQUAL 8)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_8})
  elseif (TARGET_WIDTH EQUAL 16)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_16})
  endif()

  set(ISPC_DEFINITIONS
    "-DVKL_TARGET_WIDTH=${TARGET_WIDTH}"
    ${OPTIONAL_VOLUME_DEFINITIONS_ISPC}
  )

  openvkl_add_library_ispc(${TARGET_NAME} SHARED
    api/CPUDevice.cpp
    api/CPUDevice.ispc
    iterator/DefaultIterator.cpp
    iterator/DefaultIterator.ispc
    iterator/IteratorContext.cpp
    iterator/IteratorContext.ispc
    observer/Observer.cpp
    observer/ObserverRegistry.cpp
    observer/ObserverRegistry.ispc
    sampler/Sampler.cpp
    sampler/Sampler.ispc
    ${OPTIONAL_VOLUME_SOURCES}
  )

  unset(ISPC_DEFINITIONS)
  unset(ISPC_TARGETS_OVERRIDE)

  # hidden symbols by default, so each target width library can use its internal
  # ISPC-built components without conflict
  set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
  )

  target_compile_definitions(${TARGET_NAME} PRIVATE
    "VKL_TARGET_WIDTH=${TARGET_WIDTH}"
    ${OPTIONAL_VOLUME_DEFINITIONS}
  )

  set(width_compile_options "")
  openvkl_get_compile_options_for_width(${TARGET_WIDTH} width_compile_options)
  separate_arguments(width_compile_options UNIX_COMMAND "${width_compile_options}")

  message(STATUS "using compile options for width ${TARGET_WIDTH}: ${width_compile_options}")

  target_compile_options(${TARGET_NAME} PRIVATE ${width_compile_options})

  target_include_directories(${TARGET_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>
  )

  target_link_libraries(${TARGET_NAME}
    PUBLIC
      openvkl
      $<BUILD_INTERFACE:rkcommon::rkcommon>
    PRIVATE
      embree
      $<INSTALL_INTERFACE:rkcommon::rkcommon>
  )

  openvkl_install_library(${TARGET_NAME})

  # accumulate targets to be linked into single shared library
  list(APPEND ACTIVE_WIDTH_TARGETS ${TARGET_NAME})

endforeach(TARGET_WIDTH)


# shared library linking to all width-specific targets
add_library(openvkl_module_cpu_device SHARED exports.cpp ${VKL_RESOURCE})

target_compile_definitions(openvkl_module_cpu_device PUBLIC
  ${OPTIONAL_VOLUME_DEFINITIONS}
)

foreach(TARGET_WIDTH 4 8 16)
  if (TARGET_WIDTH_ENABLED_${TARGET_WIDTH})
    target_compile_definitions(openvkl_module_cpu_device PUBLIC
      "VKL_TARGET_WIDTH_ENABLED_${TARGET_WIDTH}")
  endif()
endforeach(TARGET_WIDTH)

target_include_directories(openvkl_module_cpu_device
  INTERFACE $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>)

target_link_libraries(openvkl_module_cpu_device
  PUBLIC
    openvkl
)

openvkl_install_library(openvkl_module_cpu_device)

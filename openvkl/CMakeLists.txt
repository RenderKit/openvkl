## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_ispc)

if(WIN32)
  set(DEF_FILE common/ispc_defs.def)
endif()

set(VKL_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/include/openvkl/openvkl.rc")
set(VKL_RESOURCE ${VKL_RESOURCE} PARENT_SCOPE)

openvkl_add_library_ispc(${PROJECT_NAME} SHARED
  api/API.cpp
  api/Device.cpp

  common/Data.cpp
  common/ispc_util.ispc
  common/logging.cpp
  common/ManagedObject.cpp
  common/Traits.cpp
  common/VKLCommon.cpp

  ${DEF_FILE}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:rkcommon::rkcommon>)

set(VKL_VDB_LOG_RESOLUTION_0 "6" CACHE STRING
  "Base 2 logarithm of the root level resolution in vdb volumes.")

set(VKL_VDB_LOG_RESOLUTION_1 "5" CACHE STRING
  "Base 2 logarithm of the level 1 resolution in vdb volumes.")

set(VKL_VDB_LOG_RESOLUTION_2 "4" CACHE STRING
  "Base 2 logarithm of the level 2 resolution in vdb volumes.")

set(VKL_VDB_LOG_RESOLUTION_3 "3" CACHE STRING
  "Base 2 logarithm of the leaf level resolution in vdb volumes.")

openvkl_vdb_generate_topology()

## Configure OpenVKL installation ##

openvkl_install_library(${PROJECT_NAME})

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.isph"
)

install(DIRECTORY
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
)

install(FILES
  ${PROJECT_SOURCE_DIR}/LICENSE.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-Embree.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-OSPRay.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-TBB.txt
  ${PROJECT_SOURCE_DIR}/README.md
  ${PROJECT_SOURCE_DIR}/CHANGELOG.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR})

# needed for build-time generated version.h
target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}>
)

## Add OpenVKL devices ##

add_subdirectory(devices)

## Utilities

add_executable(vklGenerateIteratorSizeHeader
  util/vklGenerateIteratorSizeHeader.cpp
  ${VKL_RESOURCE})

target_link_libraries(vklGenerateIteratorSizeHeader PRIVATE openvkl openvkl_module_cpu_device)

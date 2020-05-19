## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

# A header-only utility library that shows how vdb volumes can be set up.

add_library(openvkl_vdb_util INTERFACE)

target_include_directories(openvkl_vdb_util
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(openvkl_vdb_util
  INTERFACE
    rkcommon::rkcommon
    openvkl
)

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/vdb_util
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  FILES_MATCHING
  PATTERN "*.h"
)

# ------------------------------------------------------------------------------

if (DEFINED OpenVDB_ROOT)
  # Monitor <PackageName>_ROOT variables, as this mechanism is used a lot
  # in OpenVDB.
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()

  # OpenVDB comes with a set of cmake scripts that help find its dependencies.
  # We must set the module path to find these scripts.
  list(APPEND CMAKE_MODULE_PATH ${OpenVDB_ROOT}/lib/cmake/OpenVDB)
  find_package(OpenVDB 7.0.0 COMPONENTS openvdb)

  if (OpenVDB_FOUND)
    # Applications should use this variable to determine if .vdb file loading is
    # enabled.
    target_compile_definitions(openvkl_vdb_util 
      INTERFACE
        OPENVKL_VDB_UTIL_OPENVDB_ENABLED)

    target_link_libraries(openvkl_vdb_util 
      INTERFACE 
        OpenVDB::openvdb)

    message(STATUS "Building with OpenVDB support: ${OpenVDB_openvdb_LIBRARY}.")
  else()
    message(STATUS "Failed to find OpenVDB at OpenVDB_ROOT (${OpenVDB_ROOT}).")
  endif()

else()
  message(STATUS "Building without OpenVDB support.")
endif()




## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## Global CMake options ##

cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_DISABLE_SOURCE_CHANGES ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

## Establish project ##

project(openvkl VERSION 0.11.0 LANGUAGES C CXX)

## Add openvkl specific macros ##

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
include(openvkl_macros)
include(openvkl_ispc)

## openvkl specific configuration ##

option(BUILD_EXAMPLES "Build example applications." ON)
option(BUILD_BENCHMARKS "Build benchmark applications." OFF)

include(GNUInstallDirs)
include(CTest) # Adds 'BUILD_TESTING' variable

if (BUILD_TESTING)
  enable_testing()
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

openvkl_configure_build_type()
openvkl_configure_global_build_flags()
openvkl_configure_ispc_isa()

set(RKCOMMON_VERSION_REQUIRED 1.4.1)
find_package(rkcommon ${RKCOMMON_VERSION_REQUIRED} REQUIRED)

find_package(embree 3.6.1 REQUIRED)
openvkl_create_embree_target()

## Build library, examples, and tests ##

add_subdirectory(openvkl)
add_subdirectory(vdb_util)

if (BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

add_subdirectory(testing)

## Configure CMake find_package() config files ##

include(CMakePackageConfigHelpers)

configure_package_config_file(
  "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/rkcommon
)

write_basic_package_version_file(
  "${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/cmake/openvkl-${PROJECT_VERSION}
)

# Must be last
include(CPack)

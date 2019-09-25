## ======================================================================== ##
## Copyright 2019 Intel Corporation                                         ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

## Global settings ##

cmake_minimum_required(VERSION 3.1)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

if(NOT CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX
      "${CMAKE_BINARY_DIR}/install"
      CACHE STRING "Final install location." FORCE)
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

## Establish project ##

project(OpenVKL_superbuild)

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)

## Superbuild options ##

if(NOT PROCESSOR_COUNT EQUAL 0)
  set(BUILD_JOBS ${PROCESSOR_COUNT} CACHE STRING "Number of build jobs '-j <n>'")
else()
  set(BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")
endif()

set(BUILD_OSPCOMMON_VERSION "master" CACHE STRING "Which version of ospcommon to build?")

option(BUILD_TBB_FROM_SOURCE "Build TBB from source or instead use one in the existing environment?" ON)

option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  ON
)

option(BUILD_GLFW "Build glfw for examples or use one in existing environment?" ON)

option(BUILD_OPENVKL_TESTING "Enable testing targets when building OpenVKL" ON)
option(BUILD_OPENVKL_BENCHMARKS "Enable benchmark targets when building OpenVKL" ON)

## Setup top-level variables ##

set(installDir ${CMAKE_INSTALL_PREFIX})

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

if (${CMAKE_VERSION} VERSION_GREATER 3.11.4)
  set(PARALLEL_JOBS_OPTS -j ${NUM_BUILD_JOBS})
endif()

option(BUILD_DEPENDENCIES_ONLY "Don't build OpenVKL itself, only its dependencies" OFF)

set(DEFAULT_BUILD_COMMAND cmake --build . --config release ${PARALLEL_JOBS_OPTS})

## Build dependencies ##

include(dep_ispc)

if (BUILD_TBB_FROM_SOURCE)
  include(dep_tbb)
endif()

if (BUILD_GLFW)
  include(dep_glfw)
endif()

include(dep_ospcommon)

## Build OpenVKL ##

set(COMPONENT_NAME openvkl)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (NOT BUILD_DEPENDENCIES_ONLY)
  ExternalProject_Add(openvkl
    PREFIX openvkl
    DOWNLOAD_COMMAND ""
    STAMP_DIR openvkl/stamp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
    BINARY_DIR openvkl/build
    CMAKE_ARGS
      -DBUILD_TESTING=${BUILD_OPENVKL_TESTING}
      -DBUILD_BENCHMARKS=${BUILD_OPENVKL_BENCHMARKS}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      -Dospcommon_DIR:PATH=${OSPCOMMON_PATH}
      -DISPC_EXECUTABLE=${ISPC_PATH}
      $<$<BOOL:${BUILD_TBB_FROM_SOURCE}>:-DOSPCOMMON_TBB_ROOT=${TBB_PATH}>
      $<$<BOOL:${BUILD_GLFW}>:-Dglfw3_DIR:PATH=${GLFW_PATH}>
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  ExternalProject_Add_StepDependencies(openvkl
  configure
    ospcommon
    ispc
    $<$<BOOL:${BUILD_GLFW}>:glfw>
  )
endif()

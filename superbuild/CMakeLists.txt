## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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

## ======================================================================== ##
## Project setup.
## ======================================================================== ##

project(OpenVKL_superbuild)

include(ExternalProject)
include(GNUInstallDirs)
include(ProcessorCount)

## Setup top-level variables ##

set(installDir ${CMAKE_INSTALL_PREFIX})

get_filename_component(INSTALL_DIR_ABSOLUTE
  ${installDir} ABSOLUTE BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

ProcessorCount(PROCESSOR_COUNT)

if(NOT PROCESSOR_COUNT EQUAL 0)
  set(BUILD_JOBS ${PROCESSOR_COUNT} CACHE STRING "Number of build jobs '-j <n>'")
else()
  set(BUILD_JOBS 4 CACHE STRING "Number of build jobs '-j <n>'")
endif()

if (${CMAKE_VERSION} VERSION_GREATER 3.11.4)
  set(PARALLEL_JOBS_OPTS -j ${BUILD_JOBS})
endif()

set(DEFAULT_BUILD_COMMAND ${CMAKE_COMMAND} --build . --config Release ${PARALLEL_JOBS_OPTS})

# We must replace separators so that ExternalProject_Add can work with the
# paths we generate.
macro(add_to_prefix_path PATH)
  list(APPEND CMAKE_PREFIX_PATH ${PATH})
  string(REPLACE ";" "|" CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}")
endmacro()

## ======================================================================== ##
## Superbuild options.
## ======================================================================== ##

option(INSTALL_IN_SEPARATE_DIRECTORIES
  "Install libraries into their own directories under CMAKE_INSTALL_PREFIX"
  OFF
)

option(BUILD_DEPENDENCIES_ONLY
  "Do not build Open VKL itself, only its dependencies"
  OFF
)

option(BUILD_OPENVKL_TESTING
  "Enable testing targets when building Open VKL"
  ON
)

option(BUILD_OPENVKL_BENCHMARKS
  "Enable benchmark targets when building Open VKL"
  OFF
)

## ======================================================================== ##
## Build dependencies.
##
## Use the BUILD_* options to enable or disable building in superbuild. If
## These options are OFF, then required dependencies must be available in the
## environment.
##
## For most of the dependencies, you may override the *_URL and *_HASH
## variables to select different versions.
## ======================================================================== ##

option(BUILD_ISPC "Build the Intel SPMD Program Compiler or search in environment?" ON)
if (BUILD_ISPC)
  set(ISPC_VERSION "1.22.0")
  set(ISPC_BASE_URL "https://github.com/ispc/ispc/releases/download/v${ISPC_VERSION}")
  if (APPLE)
    set(_ISPC_URL "${ISPC_BASE_URL}/ispc-v${ISPC_VERSION}-macOS.universal.tar.gz")
    set(_ISPC_HASH "af31ca1f02b04fbdd44b747f9e13c1df54d3e89a6e87c891ac5409b3b3a71370")
  elseif(WIN32)
    set(_ISPC_URL "${ISPC_BASE_URL}/ispc-v${ISPC_VERSION}-windows.zip")
    set(_ISPC_HASH "e597a1568675d5c5ad9cf2fe5be2653d279c74b46d0e899a01a844770a0a9ad1")
  else()
    set(_ISPC_URL "${ISPC_BASE_URL}/ispc-v${ISPC_VERSION}-linux.tar.gz")
    set(_ISPC_HASH "8c935ef7537c241a519f2632e6ffeae0988a64f21db78d403ceaa4c52517b416")
  endif()
  set(ISPC_URL "${_ISPC_URL}" CACHE STRING "URL of the ISPC archive.")
  set(ISPC_HASH "${_ISPC_HASH}" CACHE STRING "SHA256 hash of the ISPC archive.")
  include(dep_ispc)
endif()

option(BUILD_TBB "Build Intel Threading Building Blocks or search in environment?" ON)
option(BUILD_TBB_FROM_SOURCE "Build Intel Threading Building Blocks from source or use pre-built version?" OFF)
if (BUILD_TBB)
  set(TBB_VERSION "2021.11.0" CACHE STRING "TBB version to download")
  if (BUILD_TBB_FROM_SOURCE)
    if (TBB_VERSION VERSION_LESS 2021)
      message(FATAL_ERROR "Only TBB 2021 and later are supported when building TBB from source")
    endif()

    string(REGEX REPLACE "(^[0-9]+\.[0-9]+\.[0-9]+$)" "v\\1" TBB_ARCHIVE ${TBB_VERSION})
    set(_TBB_URL "https://github.com/oneapi-src/oneTBB/archive/refs/tags/${TBB_ARCHIVE}.zip")
    set(_TBB_HASH "2f0bfce641d238e80798fc48397d43821bd977d49c4e03bc785be363b7ab4742")
  else()
    if (TBB_VERSION VERSION_LESS 2021)
      set(TBB_BASE_URL "https://github.com/oneapi-src/oneTBB/releases/download/v${TBB_VERSION}/tbb-${TBB_VERSION}")
    else()
      set(TBB_BASE_URL "https://github.com/oneapi-src/oneTBB/releases/download/v${TBB_VERSION}/oneapi-tbb-${TBB_VERSION}")
    endif()
    if (APPLE)
      set(_TBB_URL "${TBB_BASE_URL}-mac.tgz")
      set(_TBB_HASH "360bcb20bcdcd01e8492c32bba6d5d5baf4bc83f77fb9dbf1ff701ac816e3b44")
      set(TBB_LIB_SUBDIR "")
    elseif(WIN32)
      set(_TBB_URL "${TBB_BASE_URL}-win.zip")
      set(_TBB_HASH "02f0e93600fba69bb1c00e5dd3f66ae58f56e5410342f6155455a95ba373b1b6")
      set(TBB_LIB_SUBDIR "intel64/vc14")
    else()
      set(_TBB_URL "${TBB_BASE_URL}-lin.tgz")
      set(_TBB_HASH "95659f4d7b1711c41ffa190561d4e5b6841efc8091549661c7a2e6207e0fa79b")
      set(TBB_LIB_SUBDIR "intel64/gcc4.8")
    endif()
  endif()
  set(TBB_URL "${_TBB_URL}" CACHE STRING "URL of the TBB archive.")
  set(TBB_HASH "${_TBB_HASH}" CACHE STRING "SHA256 hash of the TBB archive.")
  include(dep_tbb)
endif()


option(BUILD_RKCOMMON "Build rkcommon or search in environment?" ON)
if (BUILD_RKCOMMON)
  set(RKCOMMON_VERSION "v1.13.0" CACHE STRING "rkcommon version to download")
  set(RKCOMMON_URL "https://github.com/ospray/rkcommon/archive/${RKCOMMON_VERSION}.zip"
    CACHE STRING "URL of the rkcommon archive.")
  set(RKCOMMON_HASH "9d360ce89de1842d4ae81b561ce150efd4fd812bd9e8b1d42d3934c528de637c" CACHE STRING "SHA256 hash of the rkcommon archive.")
  include(dep_rkcommon)
endif()


# Embree can either be build from source or downloaded as binaries.
option(BUILD_EMBREE "Build Intel Embree or search in environment?" ON)
option(BUILD_EMBREE_FROM_SOURCE "Build Embree from source or use pre-built version? (Only used when BUILD_EMBREE=ON)" ON)
option(BUILD_EMBREE_SYCL "Build Intel Embree with SYCL support?" OFF)
if (BUILD_EMBREE)
  set(EMBREE_VERSION "v4.3.1" CACHE STRING "Embree version to download")
  if (BUILD_EMBREE_FROM_SOURCE)
    set(_EMBREE_URL "https://github.com/embree/embree/archive/${EMBREE_VERSION}.zip")
    set(_EMBREE_HASH "bdab87b285efa1a9f1f57fe74b2743c659c487fee7e32221db43a6b8f5e36e5f")
  else()
    # Embree binary package URLs do not use the "v" prefix
    string(REPLACE "v" "" EMBREE_VERSION_NUMBER ${EMBREE_VERSION})
    set(EMBREE_BASE_URL "https://github.com/embree/embree/releases/download/${EMBREE_VERSION}")
    if (APPLE)
      set(_EMBREE_URL "${EMBREE_BASE_URL}/embree-${EMBREE_VERSION_NUMBER}.x86_64.macosx.zip")
      set(_EMBREE_HASH "ffb446c5a688d961f4219f42446147632f5704374c5b2b148a1e2a20b7c99975")
    elseif (WIN32)
      set(_EMBREE_URL "${EMBREE_BASE_URL}/embree-${EMBREE_VERSION_NUMBER}.x64.windows.zip")
      set(_EMBREE_HASH "6e2b968d45c0895cda98fb44c42e214e8d382bcd08b0a548fe086b0864563c6a")
    else()
      set(_EMBREE_URL "${EMBREE_BASE_URL}/embree-${EMBREE_VERSION_NUMBER}.x86_64.linux.tar.gz")
      set(_EMBREE_HASH "375e829dc31ac5c6c579de34e0b1527e088a3b296fb9d3f90ed892d29592f845")
    endif()
  endif()
  set(EMBREE_URL "${_EMBREE_URL}"  CACHE STRING "URL of the Embree source archive.")
  set(EMBREE_HASH "${_EMBREE_HASH}" CACHE STRING "SHA256 hash for the Embree source archive.")
  include(dep_embree)
endif()


option(BUILD_GLFW "Build glfw or search in environment?" ON)
if (BUILD_GLFW)
  set(GLFW_VERSION "3.3.9")
  set(GLFW_URL "https://github.com/glfw/glfw/archive/${GLFW_VERSION}.zip"
    CACHE STRING "URL of the GLFW source archive.")
  set(GLFW_HASH "c49d895b1f32fa3e072626f6dc928887fc814f445d3ba1fbb97598fea8e48933"
    CACHE STRING "SHA256 hash of the GLFW source archive.")
  include(dep_glfw)
endif()

# We always build or download OpenVDB and all its dependencies if OpenVDB is
# enabled, and to not attempt to find them in the environment.
set(BUILD_OPENVDB_DEFAULT ON)
mark_as_advanced(BUILD_OPENVDB_DEFAULT)

option(BUILD_OPENVDB "Enable OpenVDB to support loading .vdb files?" ${BUILD_OPENVDB_DEFAULT})
option(BUILD_BLOSC "Support loading compressed .vdb files using c-BLOSC? (Only used when BUILD_OPENVDB=ON)" ON)
if (BUILD_OPENVDB)
  if (CMAKE_VERSION VERSION_LESS 3.12)
    message(WARNING "Disabling OpenVDB support because the cmake version is too low (3.12 is required)")
    set(BUILD_OPENVDB OFF)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.3)
    message(WARNING "Disabling OpenVDB support because the gcc compiler version is too low (6.3 is required)")
    set(BUILD_OPENVDB OFF)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.1)
    message(WARNING "Disabling OpenVDB support because the icc compiler version is too low (19.1 is required)")
    set(BUILD_OPENVDB OFF)
  elseif (WIN32 AND BUILD_TBB AND (NOT TBB_VERSION VERSION_LESS 2021))
    message(WARNING "Open VKL superbuild currently does not support building OpenVDB on Windows with TBB 2021")
    set(BUILD_OPENVDB OFF)
  else()
    set(ILMBASE_VERSION "2.5.10")
    set(ILMBASE_URL "https://github.com/AcademySoftwareFoundation/openexr/archive/v${ILMBASE_VERSION}.zip"
      CACHE STRING "URL of the IlmBase archive.")
    set(ILMBASE_HASH "8580ae0d1372fd54ba7ad79314253a9db2c69099eaee6472b3c1081895168c08"
      CACHE STRING "SHA256 hash of the IlmBase archive.")
    include(dep_ilmbase)

    set(ZLIB_VERSION "1.3.1")
    set(ZLIB_URL "https://www.zlib.net/fossils/zlib-${ZLIB_VERSION}.tar.gz"
      CACHE STRING "URL of the zlib archive.")
    set(ZLIB_HASH "9a93b2b7dfdac77ceba5a558a580e74667dd6fede4585b91eefb60f03b72df23"
      CACHE STRING "SHA256 hash of the zlib archive.")
    include(dep_zlib)

    set(BOOST_VERSION "1.84.0")
    string(REPLACE "." "_" BOOST_FILE_BASE "${BOOST_VERSION}")
    set(BOOST_BASE_URL "https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost")
    set(_BOOST_URL "${BOOST_BASE_URL}_${BOOST_FILE_BASE}.tar.gz")
    set(_BOOST_HASH "a5800f405508f5df8114558ca9855d2640a2de8f0445f051fa1c7c3383045724")
    set(BOOST_URL "${_BOOST_URL}" CACHE STRING "URL of the boost archive.")
    set(BOOST_HASH "${_BOOST_HASH}" CACHE STRING "SHA256 hash of the boost archive.")
    include(dep_boost)

    if (BUILD_BLOSC)
      set(BLOSC_VERSION "1.21.5")
      set(BLOSC_URL "https://github.com/Blosc/c-blosc/archive/v${BLOSC_VERSION}.zip"
        CACHE STRING "URL of the c-BLOSC archive.")
      set(BLOSC_HASH "bc022fd194e40421531d2ef69831f2793d405d98f60e759c697ccc02dad765ec"
        CACHE STRING "SHA256 hash of the c-BLOSC archive.")
      include(dep_blosc)
    endif()

    set(OPENVDB_VERSION "9.1.0")
    set(OPENVDB_BASE_URL "https://github.com/AcademySoftwareFoundation/openvdb/archive")
    set(OPENVDB_URL "${OPENVDB_BASE_URL}/v${OPENVDB_VERSION}.zip"
      CACHE STRING "URL of the OpenVDB archive.")
    set(OPENVDB_HASH "6a5e953e09828e371f8d05e20420ff182ed5149bfbcbdc92d9eca24f15093640"
      CACHE STRING "SHA256 hash of the OpenVDB archive.")
    include(dep_openvdb)
  endif()
endif()

## ======================================================================== ##
## Build OpenVKL
## ======================================================================== ##

set(COMPONENT_NAME openvkl)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

if (NOT BUILD_DEPENDENCIES_ONLY)
  # needed to support multiple CMake options provided in a single quoted string
  separate_arguments(OPENVKL_EXTRA_OPTIONS UNIX_COMMAND "${OPENVKL_EXTRA_OPTIONS}")

  ExternalProject_Add(openvkl
    PREFIX openvkl
    DOWNLOAD_COMMAND ""
    STAMP_DIR openvkl/stamp
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..
    BINARY_DIR openvkl/build
    LIST_SEPARATOR | # CMAKE_PREFIX_PATH uses alternate separator.
    CMAKE_ARGS
      -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
      -DBUILD_TESTING=${BUILD_OPENVKL_TESTING}
      -DBUILD_BENCHMARKS=${BUILD_OPENVKL_BENCHMARKS}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
      -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
      -DCMAKE_INSTALL_PREFIX=${COMPONENT_PATH}
      -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
      -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
      $<$<BOOL:${BUILD_ISPC}>:-DISPC_EXECUTABLE=${ISPC_PATH}>
      $<$<BOOL:${BUILD_TBB}>:-DRKCOMMON_TBB_ROOT=${TBB_PATH}>
      $<$<BOOL:${BUILD_OPENVDB}>:-DOpenVDB_ROOT=${OPENVDB_PATH}>
      ${OPENVKL_EXTRA_OPTIONS}
      ${OPENVKL_EXTRA_OPENVDB_OPTIONS}
    BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
    BUILD_ALWAYS OFF
  )

  ExternalProject_Add_StepDependencies(openvkl
  configure
    $<$<BOOL:${BUILD_ISPC}>:ispc>
    $<$<BOOL:${BUILD_TBB}>:tbb>
    $<$<BOOL:${BUILD_RKCOMMON}>:rkcommon>
    $<$<BOOL:${BUILD_GLFW}>:glfw>
    $<$<BOOL:${BUILD_EMBREE}>:embree>
    $<$<BOOL:${BUILD_OPENVDB}>:openvdb>
  )
endif()

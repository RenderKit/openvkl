## ======================================================================== ##
## Copyright 2020 Intel Corporation                                         ##
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

set(COMPONENT_NAME openvdb)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

# Options that are shared between the OpenVDB and Open VKL builds.
# We need to define these both for the OpenVDB dependency build and
# the main Open VKL build.
set(OPENVKL_EXTRA_OPENVDB_OPTIONS -DOpenVDB_ABI=7
                                  -DCMAKE_NO_SYSTEM_FROM_IMPORTED=ON)
mark_as_advanced(OPENVKL_EXTRA_OPENVDB_OPTIONS)


ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/build
  URL ${OPENVDB_URL}
  URL_HASH SHA256=${OPENVDB_HASH}
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX=${COMPONENT_PATH}
    -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
    -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
    -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
    -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
    -DCMAKE_BUILD_TYPE=Release
    -DOPENVDB_BUILD_BINARIES:BOOL=OFF
    -DOPENVDB_BUILD_CORE:BOOL=ON
    -DOPENVDB_BUILD_DOCS:BOOL=OFF
    -DOPENVDB_BUILD_PYTHON_MODULE:BOOL=OFF
    -DOPENVDB_BUILD_UNITTESTS:BOOL=OFF
    -DOPENVDB_CODE_COVERAGE:BOOL=OFF
    -DIlmBase_ROOT=${ILMBASE_PATH}
    -DZLIB_ROOT=${ZLIB_PATH}
    -DBoost_ROOT=${BOOST_PATH}
    -DUSE_BLOSC:BOOL=${BUILD_BLOSC}
    -DCONCURRENT_MALLOC=None 
    -DDISABLE_CMAKE_SEARCH_PATHS:BOOL=ON
    $<$<BOOL:${BUILD_BLOSC}>:-DBlosc_ROOT=${BLOSC_PATH}>
    $<$<BOOL:${BUILD_TBB}>:-DTBB_ROOT=${TBB_PATH}>
    ${OPENVKL_EXTRA_OPENVDB_OPTIONS}
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  INSTALL_COMMAND cmake --build . --target install
  BUILD_ALWAYS OFF
)

ExternalProject_Add_StepDependencies(openvdb
  configure
    boost
    ilmbase
    $<$<BOOL:${BUILD_BLOSC}>:c-blosc>
    $<$<BOOL:${BUILD_TBB}>:tbb>
)

set(OPENVDB_PATH ${COMPONENT_PATH})

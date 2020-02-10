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

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}/build
  URL "https://github.com/AcademySoftwareFoundation/openvdb/archive/${BUILD_OPENVDB_VERSION}.zip"
  CMAKE_ARGS
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -DCMAKE_INSTALL_PREFIX:PATH=${COMPONENT_PATH}
    -DCMAKE_INSTALL_INCLUDEDIR=${CMAKE_INSTALL_INCLUDEDIR}
    -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
    -DCMAKE_INSTALL_DOCDIR=${CMAKE_INSTALL_DOCDIR}
    -DCMAKE_INSTALL_BINDIR=${CMAKE_INSTALL_BINDIR}
    -DCMAKE_BUILD_TYPE=Release
    -DOPENVDB_BUILD_BINARIES=ON
    -DOPENVDB_BUILD_CORE=ON
    -DOPENVDB_BUILD_DOCS=OFF
    -DOPENVDB_BUILD_PYTHON_MODULE=OFF
    -DOPENVDB_BUILD_UNITTESTS=OFF
    -DOPENVDB_BUILD_VDB_VIEW=OFF
    -DOPENVDB_CODE_COVERAGE=OFF
    -DCMAKE_NO_SYSTEM_FROM_IMPORTED:BOOL=TRUE
    -DIlmBase_ROOT=${ILMBASE_ROOT}
    -DUSE_BLOSC=${BUILD_BLOSC}
    -DZLIB_ROOT=${ZLIB_ROOT}
    -DBOOST_ROOT=${BOOST_ROOT}
    $<$<BOOL:${BUILD_BLOSC}>:-DBlosc_ROOT=${Blosc_ROOT}>
    $<$<BOOL:${BUILD_TBB_FROM_SOURCE}>:-DTBB_ROOT=${TBB_PATH}>
    -DCONCURRENT_MALLOC=Tbbmalloc
  BUILD_COMMAND ${DEFAULT_BUILD_COMMAND}
  BUILD_ALWAYS OFF
)

ExternalProject_Add_StepDependencies(openvdb
  configure
    zlib
    boost
    ilmbase
    $<$<BOOL:${BUILD_TBB_FROM_SOURCE}>:tbb>
    $<$<BOOL:${BUILD_BLOSC}>:c-blosc>
)

set(OPENVDB_ROOT ${COMPONENT_PATH})

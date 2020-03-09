## ======================================================================== ##
## Copyright 2019-2020 Intel Corporation                                    ##
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

set(COMPONENT_NAME tbb)

set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE})
if (INSTALL_IN_SEPARATE_DIRECTORIES)
  set(COMPONENT_PATH ${INSTALL_DIR_ABSOLUTE}/${COMPONENT_NAME})
endif()

ExternalProject_Add(${COMPONENT_NAME}
  PREFIX ${COMPONENT_NAME}
  DOWNLOAD_DIR ${COMPONENT_NAME}
  STAMP_DIR ${COMPONENT_NAME}/stamp
  SOURCE_DIR ${COMPONENT_NAME}/src
  BINARY_DIR ${COMPONENT_NAME}
  URL ${TBB_URL}
  URL_HASH SHA256=${TBB_HASH}
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND "${CMAKE_COMMAND}" -E copy_directory
    <SOURCE_DIR>/tbb/include
    ${COMPONENT_PATH}/include
  BUILD_ALWAYS OFF
)

# We copy the libraries into the main lib dir. This makes it easier
# to set the correct library path.
ExternalProject_Add_Step(${COMPONENT_NAME} install_lib
  COMMAND "${CMAKE_COMMAND}" -E copy_directory 
  <SOURCE_DIR>/tbb/lib/${TBB_LIB_SUBDIR} ${COMPONENT_PATH}/lib
  DEPENDEES install
)

if (WIN32)
  # DLLs on Windows are in the bin subdirectory.
  ExternalProject_Add_Step(${COMPONENT_NAME} install_dll
    COMMAND "${CMAKE_COMMAND}" -E copy_directory 
    <SOURCE_DIR>/tbb/bin/${TBB_LIB_SUBDIR} ${COMPONENT_PATH}/bin
    DEPENDEES install_lib
  )
endif()

set(TBB_PATH ${COMPONENT_PATH})

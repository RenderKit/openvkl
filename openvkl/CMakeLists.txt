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

include(openvkl_ispc)

if(WIN32)
  set(DEF_FILE common/ispc_defs.def)
endif()

openvkl_add_library_ispc(${PROJECT_NAME} SHARED
  api/API.cpp
  api/Driver.cpp

  common/Data.cpp
  common/ispc_util.ispc
  common/logging.cpp
  common/ManagedObject.cpp
  common/Observer.cpp
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

target_link_libraries(${PROJECT_NAME} PRIVATE ospcommon::ospcommon)


set(VKL_VDB_LOG_RESOLUTION "6;5;4;3" CACHE STRING
  "Base-2 logarithm of the resolution for each level in the tree.")
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

## Version header ##

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/version.h
  @ONLY
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}>
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

## Add OpenVKL drivers ##

add_subdirectory(drivers)

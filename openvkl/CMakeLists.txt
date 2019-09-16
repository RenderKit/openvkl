## ======================================================================== ##
## Copyright 2018 Intel Corporation                                         ##
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
  common/VKLCommon.cpp

  ${DEF_FILE}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE ospcommon::ospcommon)

## Configure OpenVKL installation ##

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}_Exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT ${PROJECT_NAME}_Exports
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
  NAMESPACE openvkl::
)

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.isph"
)

## Add OpenVKL drivers ##

add_subdirectory(drivers)

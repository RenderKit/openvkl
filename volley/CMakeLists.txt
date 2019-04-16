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

## Configure Volley library build ##
add_library(volley SHARED
  api/API.cpp
  api/Driver.cpp

  common/Data.cpp
  common/logging.cpp
  common/ManagedObject.cpp
  common/VLYCommon.cpp
)

target_include_directories(volley
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)

## Include ospcommon dependency ##
# TODO: ospray linked here to fix issue with ospcommon include paths
target_link_libraries(volley
  PUBLIC
    ospray::ospray
    ospray::ospray_common
)

## Configure Volley installation ##
install(TARGETS volley
  EXPORT volleyConfig
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT volleyConfig DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/volley)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/volley/include/volley
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING PATTERN "*.h"
)

## Add Volley drivers ##
add_subdirectory(drivers)

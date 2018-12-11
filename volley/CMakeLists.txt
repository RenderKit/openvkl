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

## Set global include directories ##
include_directories(
  # for ospcommon
  ${OSPRAY_INCLUDE_DIRS}

  ${CMAKE_SOURCE_DIR}/volley/include
  ${CMAKE_SOURCE_DIR}/volley
  ${CMAKE_SOURCE_DIR}
  ${PROJECT_BINARY_DIR}
)

## Build the Volley library ##
set(VOLLEY_SOURCES
  api/API.cpp
  api/Driver.cpp

  common/logging.cpp
  common/VLYCommon.cpp
)

set(VOLLEY_LIBRARIES
  # for ospcommon
  ${OSPRAY_LIBRARIES}

  ${CMAKE_THREAD_LIBS_INIT}
  ${CMAKE_DL_LIBS}
)

ospray_create_library(volley
  ${VOLLEY_SOURCES}
LINK
  ${VOLLEY_LIBRARIES}
COMPONENT
  lib
)

## Build Volley drivers ##
add_subdirectory(drivers)

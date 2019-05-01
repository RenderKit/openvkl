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

add_library(volley_testing SHARED
  volume/ProceduralStructuredVolume.cpp
)

target_link_libraries(volley_testing PUBLIC
  volley
  ospray::ospray_common
)

target_include_directories(volley_testing
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

install(TARGETS volley_testing
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't build tests
add_subdirectory(external/benchmark)

add_subdirectory(apps)

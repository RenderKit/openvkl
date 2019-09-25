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

add_library(openvkl_testing INTERFACE)

target_link_libraries(openvkl_testing INTERFACE
  openvkl
  ospcommon::ospcommon
)

target_include_directories(openvkl_testing
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (BUILD_BENCHMARKS)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE) # don't build tests
  add_subdirectory(external/benchmark)
endif()

add_subdirectory(apps)

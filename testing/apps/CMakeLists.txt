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

# Benchmarks
add_executable(vlyBenchmarkStructuredVolume
  vlyBenchmarkStructuredVolume.cpp
)

target_link_libraries(vlyBenchmarkStructuredVolume
  benchmark
  volley_testing
)

install(TARGETS vlyBenchmarkStructuredVolume
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Functional tests
add_executable(vlyTests
  vlyTests.cpp
  tests/ray_iterator_intervals.cpp
  tests/simd_type_conversion.cpp
  tests/structured_volume_sampling.cpp
  tests/vectorized_sampling.cpp
)

target_link_libraries(vlyTests
  volley_testing
)

install(TARGETS vlyTests
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
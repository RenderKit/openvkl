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

# Benchmarks
if (BUILD_BENCHMARKS)
  # Structured volumes
  add_executable(vklBenchmarkStructuredVolume
    vklBenchmarkStructuredVolume.cpp
  )

  target_link_libraries(vklBenchmarkStructuredVolume
    benchmark
    openvkl_testing
  )

  install(TARGETS vklBenchmarkStructuredVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Unstructured volumes
  add_executable(vklBenchmarkUnstructuredVolume
    vklBenchmarkUnstructuredVolume.cpp
  )

  target_link_libraries(vklBenchmarkUnstructuredVolume
    benchmark
    openvkl_testing
  )

  install(TARGETS vklBenchmarkUnstructuredVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Functional tests
if (BUILD_TESTING)
  include_directories_ispc(${PROJECT_SOURCE_DIR}/openvkl/include
    ${PROJECT_SOURCE_DIR}/openvkl/drivers/ispc)

  openvkl_add_executable_ispc(vklTests
    vklTests.cpp
    tests/hit_iterator.cpp
    tests/interval_iterator.cpp
    tests/simd_conformance.cpp
    tests/simd_conformance.ispc
    tests/simd_type_conversion.cpp
    tests/structured_volume_gradients.cpp
    tests/structured_regular_volume_sampling.cpp
    tests/structured_spherical_volume_sampling.cpp
    tests/structured_spherical_volume_bounding_box.cpp
    tests/structured_volume_value_range.cpp
    tests/unstructured_volume_gradients.cpp
    tests/unstructured_volume_sampling.cpp
    tests/unstructured_volume_value_range.cpp
    tests/vectorized_gradients.cpp
    tests/vectorized_hit_iterator.cpp
    tests/vectorized_interval_iterator.cpp
    tests/vectorized_sampling.cpp
    tests/amr_volume_sampling.cpp
    tests/amr_volume_value_range.cpp
  )

  target_include_directories(vklTests PRIVATE ${ISPC_TARGET_DIR})

  target_link_libraries(vklTests PRIVATE openvkl_testing openvkl_module_ispc_driver)

  # Needed for SIMD conformance tests
  foreach(TARGET_NAME "openvkl_module_ispc_driver_4"
                      "openvkl_module_ispc_driver_8"
                      "openvkl_module_ispc_driver_16")
    if (TARGET ${TARGET_NAME})
      target_link_libraries(vklTests PRIVATE ${TARGET_NAME})
    endif()
  endforeach()

  install(TARGETS vklTests
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  ## Expose tests to CTest ##

  add_test(NAME "simd_conformance"    COMMAND vklTests "[simd_conformance]")
  add_test(NAME "interval_iterators"  COMMAND vklTests "[interval_iterators]")
  add_test(NAME "hit_iterators"       COMMAND vklTests "[hit_iterators]")
  add_test(NAME "volume_bounding_box" COMMAND vklTests "[volume_bounding_box]")
  add_test(NAME "volume_gradients"    COMMAND vklTests "[volume_gradients]")
  add_test(NAME "volume_sampling"     COMMAND vklTests "[volume_sampling]")
  add_test(NAME "volume_value_range"  COMMAND vklTests "[volume_value_range]")
endif()

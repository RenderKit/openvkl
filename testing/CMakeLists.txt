## Copyright 2019-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(openvkl_testing STATIC
  apps/AppInit.cpp
)

target_link_libraries(openvkl_testing PUBLIC
  openvkl
  rkcommon::rkcommon
  openvkl_utility
)

target_include_directories(openvkl_testing
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if (BUILD_BENCHMARKS)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)

  add_subdirectory(external/benchmark)
endif()

add_subdirectory(apps)

## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

add_library(openvkl_utility_usda INTERFACE)

target_include_directories(openvkl_utility_usda
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

install(DIRECTORY
  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/utility/usda
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/utility
  FILES_MATCHING
  PATTERN "*.h"
)

## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.1)

add_library(openvkl_utility INTERFACE)

add_subdirectory(temporal_compression)
target_link_libraries(openvkl_utility INTERFACE openvkl_utility_temporal_compression)

add_subdirectory(usda)
target_link_libraries(openvkl_utility INTERFACE openvkl_utility_usda)

add_subdirectory(vdb)
target_link_libraries(openvkl_utility INTERFACE openvkl_utility_vdb)


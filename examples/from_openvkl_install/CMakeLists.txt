## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# NOTE: This CMakeLists.txt is intended to be used to exercise an OpenVKL
#       install and demonstrate how external applications can build against
#       OpenVKL using CMake.
#
#       Once you have done a 'make install' of an existing OpenVKL
#       build, create a separate build directory and invoke CMake on this
#       directory. If you have 'openvkl_DIR' setup correctly to point to where
#       you just installed OpenVKL, then this should built the vklTutorial app
#       from that install and NOT use your local build.

cmake_minimum_required(VERSION 3.1)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

project(vklTutorial LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(openvkl REQUIRED)

add_executable(vklTutorial ${CMAKE_CURRENT_SOURCE_DIR}/../vklTutorial.c)
target_link_libraries(vklTutorial openvkl::openvkl)

add_executable(vklTutorialLinkCPUDevice ${CMAKE_CURRENT_SOURCE_DIR}/../vklTutorial.c)
target_link_libraries(vklTutorialLinkCPUDevice openvkl::openvkl_module_cpu_device)

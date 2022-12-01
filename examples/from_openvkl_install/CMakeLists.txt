## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# NOTE: This CMakeLists.txt is intended to be used to exercise an OpenVKL
#       install and demonstrate how external applications can build against
#       OpenVKL using CMake.
#
#       Once you have done a 'make install' of an existing OpenVKL
#       build, create a separate build directory and invoke CMake on this
#       directory. If you have 'openvkl_DIR' setup correctly to point to where
#       you just installed OpenVKL, then this should built
#       the vklTutorialCPU/vklTutorialGPU app
#       from that install and NOT use your local build.

cmake_minimum_required(VERSION 3.1)

project(vklTutorial)

cmake_policy(SET CMP0074 NEW)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

find_package(openvkl REQUIRED)

if (TARGET openvkl::openvkl_module_cpu_device)
  add_executable(vklTutorialCPU ${CMAKE_CURRENT_SOURCE_DIR}/../vklTutorialCPU.c)
  set_target_properties(vklTutorialCPU
                        PROPERTIES
                        C_STANDARD 99
                        C_STANDARD_REQUIRED ON
                        C_EXTENSIONS OFF)
  target_link_libraries(vklTutorialCPU PRIVATE openvkl::openvkl openvkl::openvkl_module_cpu_device)
endif()

if (TARGET openvkl::openvkl_module_gpu_device)
  find_package(rkcommon REQUIRED)
  find_package(ispcrt REQUIRED)

  # rkcommon includes FindTBB.cmake, needed by Embree
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${rkcommon_DIR})

  find_package(embree REQUIRED)

  add_executable(vklTutorialGPU ${CMAKE_CURRENT_SOURCE_DIR}/../vklTutorialGPU.cpp)
  target_link_libraries(vklTutorialGPU PRIVATE openvkl::openvkl openvkl::openvkl_module_gpu_device)
endif()

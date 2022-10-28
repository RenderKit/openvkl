## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_subdirectory(cpu)

if ((NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64") AND
    (NOT WIN32))
  option(OPENVKL_ENABLE_DEVICE_GPU "Enables GPU device." ON)
else()
  option(OPENVKL_ENABLE_DEVICE_GPU "Enables GPU device." OFF)
endif()

if (${OPENVKL_ENABLE_DEVICE_GPU})
  add_subdirectory(gpu)
endif()

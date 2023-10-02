## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_subdirectory(cpu)

option(OPENVKL_ENABLE_DEVICE_GPU "Enables GPU device." OFF)

if (${OPENVKL_ENABLE_DEVICE_GPU})
  add_subdirectory(gpu)
endif()

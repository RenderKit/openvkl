## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# add all devices
file(GLOB devices RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(device ${devices})
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${device}/CMakeLists.txt)
    add_subdirectory(${device})
  endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${device}/CMakeLists.txt)
endforeach(device ${devices})

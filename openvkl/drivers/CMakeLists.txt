## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# add all drivers
file(GLOB drivers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(driver ${drivers})
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${driver}/CMakeLists.txt)
    add_subdirectory(${driver})
  endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${driver}/CMakeLists.txt)
endforeach(driver ${drivers})

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

include(openvkl_ispc)
include(openvkl_macros)

include_directories_ispc(${PROJECT_SOURCE_DIR}/openvkl/include)

if (WIN32)
  set(DEFS_FILE ispc_driver_defs.def)
endif()

# ISPC-side build (all ISAs)
openvkl_add_library_ispc(openvkl_module_ispc_driver_ispc OBJECT
  simd_conformance.ispc
  iterator/DefaultIterator.ispc
  iterator/GridAcceleratorIterator.ispc
  iterator/UnstructuredIterator.ispc
  value_selector/ValueSelector.ispc
  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/UnstructuredVolume.ispc
  volume/amr/AMRVolume.ispc
  volume/amr/CellRef.ispc
  volume/amr/DualCell.ispc
  volume/amr/method_current.ispc
  volume/amr/method_finest.ispc
  volume/amr/method_octant.ispc
  ${DEFS_FILE}
)

set_target_properties(openvkl_module_ispc_driver_ispc PROPERTIES
  LINKER_LANGUAGE CXX)

# keep track of objects, since TARGET_OBJECTS<> is not available for ISPC-built
# libraries, and we therefore can't just link with this target
set(openvkl_module_ispc_driver_ispc_OBJECTS ${ISPC_OBJECTS})

target_include_directories(openvkl_module_ispc_driver_ispc
  PUBLIC
    ${CMAKE_SOURCE_DIR}
  INTERFACE
    ${PROJECT_BINARY_DIR}
)

# width-specific builds

# common code between all targets will only be built for the first active target
# width
set(TARGET_WIDTH_BUILT_COMMON FALSE)

foreach(TARGET_WIDTH 4 8 16)

  openvkl_get_width_enabled(${TARGET_WIDTH} TARGET_WIDTH_ENABLED)

  if(NOT ${TARGET_WIDTH_ENABLED})
    continue()
  endif()

  set(TARGET_NAME "openvkl_module_ispc_driver_${TARGET_WIDTH}")

  add_library(${TARGET_NAME} OBJECT
    api/ISPCDriver.cpp
    iterator/DefaultIterator.cpp
    iterator/GridAcceleratorIterator.cpp
    iterator/UnstructuredIterator.cpp
    value_selector/ValueSelector.cpp
    volume/StructuredRegularVolume.cpp
    volume/StructuredSphericalVolume.cpp
    volume/UnstructuredVolume.cpp
    volume/amr/AMRVolume.cpp
    ${DEFS_FILE}
  )

  target_compile_definitions(${TARGET_NAME} PRIVATE
    "TARGET_WIDTH_ENABLED_${TARGET_WIDTH}")

  if(NOT ${TARGET_WIDTH_BUILT_COMMON})
    set(TARGET_WIDTH_BUILT_COMMON TRUE)
    target_compile_definitions(${TARGET_NAME} PRIVATE
      "TARGET_WIDTH_ENABLED_COMMON")
  endif()

  set(width_compile_options "")
  openvkl_get_compile_options_for_width(${TARGET_WIDTH} width_compile_options)
  separate_arguments(width_compile_options UNIX_COMMAND "${width_compile_options}")

  message("using compile options for width ${TARGET_WIDTH}: ${width_compile_options}")

  target_compile_options(${TARGET_NAME} PRIVATE ${width_compile_options})

  target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_SOURCE_DIR})

  target_link_libraries(${TARGET_NAME}
    PUBLIC
      openvkl
      openvkl_module_ispc_driver_ispc
      ospcommon::ospcommon
      embree
  )

  # accumulate targets to be linked into single shared library
  list(APPEND ACTIVE_WIDTH_TARGETS ${TARGET_NAME})

endforeach(TARGET_WIDTH)


# shared library linking to all width-specific targets
add_library(openvkl_module_ispc_driver SHARED
  volume/amr/AMRAccel.cpp
  volume/amr/AMRData.cpp
  ${openvkl_module_ispc_driver_ispc_OBJECTS}
)

target_link_libraries(openvkl_module_ispc_driver
  PRIVATE
    ${ACTIVE_WIDTH_TARGETS}
)

install(TARGETS openvkl_module_ispc_driver
  EXPORT ${PROJECT_NAME}_Exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

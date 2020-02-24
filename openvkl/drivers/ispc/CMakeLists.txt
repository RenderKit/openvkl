## ======================================================================== ##
## Copyright 2019-2020 Intel Corporation                                    ##
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

# width-specific builds
foreach(TARGET_WIDTH 4 8 16)

  openvkl_get_width_enabled(${TARGET_WIDTH} TARGET_WIDTH_ENABLED)

  set(TARGET_WIDTH_ENABLED_${TARGET_WIDTH} ${TARGET_WIDTH_ENABLED})

  if(NOT ${TARGET_WIDTH_ENABLED})
    continue()
  endif()

  set(TARGET_NAME "openvkl_module_ispc_driver_${TARGET_WIDTH}")

  if (TARGET_WIDTH EQUAL 4)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_4})
  elseif (TARGET_WIDTH EQUAL 8)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_8})
  elseif (TARGET_WIDTH EQUAL 16)
    set(ISPC_TARGETS_OVERRIDE ${OPENVKL_ISPC_TARGET_LIST_16})
  endif()

  set(ISPC_DEFINITIONS "-DVKL_TARGET_WIDTH=${TARGET_WIDTH}")

  openvkl_add_library_ispc(${TARGET_NAME} SHARED
    api/ISPCDriver.cpp
    api/ISPCDriver.ispc
    iterator/DefaultIterator.cpp
    iterator/DefaultIterator.ispc
    iterator/GridAcceleratorIterator.cpp
    iterator/GridAcceleratorIterator.ispc
    iterator/UnstructuredIterator.cpp
    iterator/UnstructuredIterator.ispc
    value_selector/ValueSelector.cpp
    value_selector/ValueSelector.ispc
    volume/amr/AMRAccel.cpp
    volume/amr/AMRData.cpp
    volume/amr/AMRVolume.cpp
    volume/amr/AMRVolume.ispc
    volume/amr/CellRef.ispc
    volume/amr/DualCell.ispc
    volume/amr/method_current.ispc
    volume/amr/method_finest.ispc
    volume/amr/method_octant.ispc
    volume/GridAccelerator.ispc
    volume/SharedStructuredVolume.ispc
    volume/StructuredRegularVolume.cpp
    volume/StructuredSphericalVolume.cpp
    volume/UnstructuredVolume.cpp
    volume/UnstructuredVolume.ispc
  )

  unset(ISPC_DEFINITIONS)
  unset(ISPC_TARGETS_OVERRIDE)

  # hidden symbols by default, so each target width library can use its internal
  # ISPC-built components without conflict
  set_target_properties(${TARGET_NAME} PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN 1
  )

  target_compile_definitions(${TARGET_NAME} PRIVATE
    "VKL_TARGET_WIDTH=${TARGET_WIDTH}")

  set(width_compile_options "")
  openvkl_get_compile_options_for_width(${TARGET_WIDTH} width_compile_options)
  separate_arguments(width_compile_options UNIX_COMMAND "${width_compile_options}")

  message(STATUS "using compile options for width ${TARGET_WIDTH}: ${width_compile_options}")

  target_compile_options(${TARGET_NAME} PRIVATE ${width_compile_options})

  target_include_directories(${TARGET_NAME}
    PUBLIC
      $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
      $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>
  )

  target_link_libraries(${TARGET_NAME}
    PUBLIC
      openvkl
      ospcommon::ospcommon
    PRIVATE
      embree
  )

  install(TARGETS ${TARGET_NAME}
    EXPORT ${PROJECT_NAME}_Exports
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  # accumulate targets to be linked into single shared library
  list(APPEND ACTIVE_WIDTH_TARGETS ${TARGET_NAME})

endforeach(TARGET_WIDTH)


# shared library linking to all width-specific targets
add_library(openvkl_module_ispc_driver SHARED exports.cpp)

foreach(TARGET_WIDTH 4 8 16)
  if (TARGET_WIDTH_ENABLED_${TARGET_WIDTH})
    target_compile_definitions(openvkl_module_ispc_driver PUBLIC
      "VKL_TARGET_WIDTH_ENABLED_${TARGET_WIDTH}")
  endif()
endforeach(TARGET_WIDTH)

target_include_directories(openvkl_module_ispc_driver
  INTERFACE $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>)

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

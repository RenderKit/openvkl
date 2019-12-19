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

include_directories_ispc(${PROJECT_SOURCE_DIR}/openvkl/include)

if (WIN32)
  set(DEFS_FILE ispc_driver_defs.def)
endif()

openvkl_add_library_ispc(openvkl_module_ispc_driver SHARED
  simd_conformance.ispc
  api/ISPCDriver.cpp
  iterator/DefaultIterator.cpp
  iterator/DefaultIterator.ispc
  iterator/GridAcceleratorIterator.cpp
  iterator/GridAcceleratorIterator.ispc
  iterator/UnstructuredIterator.cpp
  iterator/UnstructuredIterator.ispc
  value_selector/ValueSelector.cpp
  value_selector/ValueSelector.ispc
  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/StructuredRegularVolume.cpp
  volume/StructuredSphericalVolume.cpp
  volume/UnstructuredVolume.cpp
  volume/UnstructuredVolume.ispc
  volume/amr/AMRAccel.cpp
  volume/amr/AMRData.cpp
  volume/amr/AMRVolume.cpp
  volume/amr/AMRVolume.ispc
  volume/amr/CellRef.ispc
  volume/amr/DualCell.ispc
  volume/amr/method_current.ispc
  volume/amr/method_finest.ispc
  volume/amr/method_octant.ispc
  ${DEFS_FILE}
)

target_include_directories(openvkl_module_ispc_driver
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
)

target_link_libraries(openvkl_module_ispc_driver
PUBLIC
  openvkl
PRIVATE
  ospcommon::ospcommon
  $<BUILD_INTERFACE:embree>
)

install(TARGETS openvkl_module_ispc_driver
  EXPORT ${PROJECT_NAME}_Exports
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

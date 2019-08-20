## ======================================================================== ##
## Copyright 2018 Intel Corporation                                         ##
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

openvkl_add_library_ispc(openvkl_module_ispc_driver SHARED
  simd_conformance.ispc
  api/ISPCDriver.cpp
  iterator/GridAcceleratorIterator.cpp
  iterator/GridAcceleratorIterator.ispc
  samples_mask/SamplesMask.cpp
  samples_mask/GridAcceleratorSamplesMask.cpp
  samples_mask/GridAcceleratorSamplesMask.ispc
  volume/GridAccelerator.ispc
  volume/SharedStructuredVolume.ispc
  volume/StructuredRegularVolume.cpp
)

target_include_directories(openvkl_module_ispc_driver PUBLIC
  $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
  $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
)

target_link_libraries(openvkl_module_ispc_driver PRIVATE openvkl ospcommon)

install(TARGETS openvkl_module_ispc_driver
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

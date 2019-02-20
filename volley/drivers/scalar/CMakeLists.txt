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

option(BUILD_SCALAR_DRIVER "Build scalar driver" ON)
if (BUILD_SCALAR_DRIVER)

  add_library(volley_module_scalar_driver SHARED
    api/ScalarDriver.cpp
    iterator/DumbRayIterator.cpp
    samples_mask/SamplesMask.cpp
    volume/StructuredVolume.cpp
    volume/WaveletAnalyticalVolume.cpp
    volume/WaveletProceduralVolume.cpp
  )

  target_link_libraries(volley_module_scalar_driver PRIVATE volley)

  install(TARGETS volley_module_scalar_driver
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

endif (BUILD_SCALAR_DRIVER)

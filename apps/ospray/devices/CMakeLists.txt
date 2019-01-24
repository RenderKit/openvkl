## ======================================================================== ##
## Copyright 2009-2017 Intel Corporation                                    ##
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

option(BUILD_SCALAR_VOLLEY_DEVICE "Build scalar volley device" ON)

if(BUILD_SCALAR_VOLLEY_DEVICE)

  # will need to be uncommented if this code is moved outside of Volley
  #find_package(volley REQUIRED)

  ospray_create_library(ospray_module_scalar_volley_device
    moduleInit.cpp

    api/ScalarVolleyDevice.cpp

    camera/Camera.cpp

    common/Data.cpp
    common/Model.cpp

    fb/FrameBuffer.cpp
    fb/Tile.cpp

    render/Renderer.cpp
    render/VolumeRenderer.cpp
    render/VolumeRendererDefault.cpp
    render/VolumeRendererStream.cpp
    render/VolumeRendererVolleyIntegration.cpp

    transferFunction/TransferFunction.cpp

    volume/BlockBrickedVolume.cpp
    volume/StructuredVolume.cpp
    volume/VolleyVolumeWrapper.cpp
    volume/Volume.cpp

  LINK
    ${OSPRAY_LIBRARIES}
  )

  target_link_libraries(ospray_module_scalar_volley_device volley)

  target_include_directories(ospray_module_scalar_volley_device
  PRIVATE
    ${OSPRAY_INCLUDE_DIRS}
  )

endif()

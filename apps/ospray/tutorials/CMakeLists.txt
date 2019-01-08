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

## ImGui dependency ##
add_subdirectory(imgui)

## OpenGL dependency ##
set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

## GLFW dependency ##
find_package(glfw3 REQUIRED)

## OSPRay-based Volley tutorial ##
ospray_create_application(ospTutorialVolleyVolume
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  ospTutorialVolleyVolume.cpp
  imgui_impl_glfw_gl3.cpp
LINK
  glfw
  ospray_imgui
  ospray_testing_volley
  ospray_module_scalar_volley_device
  ${OPENGL_LIBRARIES}
)

## mute OSX Mojave OpenGL deprecation warnings ##
target_compile_definitions(ospTutorialVolleyVolume PRIVATE GL_SILENCE_DEPRECATION)

install(TARGETS ospTutorialVolleyVolume
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

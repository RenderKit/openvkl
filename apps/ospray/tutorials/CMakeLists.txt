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

## OSPRay-based Volley tutorial, using ISPC device extensions ##
add_executable(ospTutorialISPCVolley
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  OSPRayWindow.cpp
  TransferFunctionWidget.cpp
  ospTutorialISPCVolley.cpp
  imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospTutorialISPCVolley
  volley
  volley_testing
  glfw
  ospray_imgui
  ospray::ospray_testing
  ospray_module_ispc_volley
  ${OPENGL_LIBRARIES}
)

target_compile_definitions(ospTutorialISPCVolley PRIVATE GL_SILENCE_DEPRECATION)

install(TARGETS ospTutorialISPCVolley
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

## OSPRay-based Volley benchmark ##
add_executable(ospBenchmarkVolley
  ArcballCamera.cpp
  OSPRayWindow.cpp
  ospBenchmarkVolley.cpp
)

target_link_libraries(ospBenchmarkVolley
  benchmark
  volley_testing
  ospray::ospray_testing
  ospray_module_ispc_volley
)

install(TARGETS ospBenchmarkVolley
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
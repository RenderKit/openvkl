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

## Cereal dependency ##
add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE ./cereal/include)

## OSPRay-based VKL example, using ISPC device extensions ##
add_executable(ospExampleVKL
  ArcballCamera.cpp
  GLFWOSPRayWindow.cpp
  OSPRayWindow.cpp
  OSPRayVKLTestScene.cpp
  TransferFunctionWidget.cpp
  ospExampleVKL.cpp
  imgui_impl_glfw_gl3.cpp
)

target_link_libraries(ospExampleVKL
  openvkl_testing
  glfw
  cereal
  ospray_imgui
  ospray::ospray_testing
  ${OPENGL_LIBRARIES}
)

target_compile_definitions(ospExampleVKL PRIVATE GL_SILENCE_DEPRECATION)

install(TARGETS ospExampleVKL
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

## OSPRay-based VKL benchmark ##
add_executable(ospBenchmarkVKL
  ArcballCamera.cpp
  OSPRayWindow.cpp
  OSPRayVKLTestScene.cpp
  ospBenchmarkVKL.cpp
)

target_link_libraries(ospBenchmarkVKL
  benchmark
  openvkl_testing
  ospray::ospray_testing
)

install(TARGETS ospBenchmarkVKL
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

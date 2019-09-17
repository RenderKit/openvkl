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

## OpenGL dependency ##

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

## GLFW dependency ##

find_package(glfw3 REQUIRED)

## Example renderers ##

include_directories_ispc(
  ${CMAKE_SOURCE_DIR}/openvkl/include
  ${CMAKE_SOURCE_DIR}/openvkl/drivers/ispc/math
)

openvkl_add_library_ispc(vkl_example_renderers STATIC
  renderers/Renderer.cpp
  renderers/Renderer.ih
  renderers/Renderer.ispc

  renderers/DensityPathTracer.cpp
  renderers/DensityPathTracer.ispc
  renderers/HitIterator.cpp
  renderers/HitIterator.ispc
  renderers/RayMarchIterator.cpp
  renderers/RayMarchIterator.ispc
)

target_link_libraries(vkl_example_renderers PUBLIC openvkl ospcommon::ospcommon)

## Interactive example app ##

add_executable(vklExamples
  window/ArcballCamera.cpp
  window/VKLWindow.cpp
  AppInit.cpp
  vklExamples.cpp

  # ImGui
  imgui/imgui.cpp
  imgui/imgui_draw.cpp
  imgui/imgui_demo.cpp

  # ImGui dependant
  window/GLFWVKLWindow.cpp
  window/imgui_impl_glfw_gl3.cpp
  window/TransferFunctionWidget.cpp
)

target_link_libraries(vklExamples PRIVATE
  openvkl_testing
  glfw
  vkl_example_renderers
  ${OPENGL_LIBRARIES}
)

target_include_directories(vklExamples
PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/imgui
)

target_compile_definitions(vklExamples PRIVATE GL_SILENCE_DEPRECATION)

install(TARGETS vklExamples RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

## Benchmark app ##

if (BUILD_BENCHMARKS)
  add_executable(vklBenchmark
    window/ArcballCamera.cpp
    window/VKLWindow.cpp
    AppInit.cpp
    vklBenchmark.cpp
  )

  target_link_libraries(vklBenchmark
  PRIVATE
    benchmark
    openvkl_testing
    vkl_example_renderers
  )

  target_include_directories(vklBenchmark PRIVATE ${CMAKE_CURRENT_LIST_DIR})

  install(TARGETS vklBenchmark RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

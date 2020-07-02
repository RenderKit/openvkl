## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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

target_include_directories(vkl_example_renderers PRIVATE ${ISPC_TARGET_DIR})

target_link_libraries(vkl_example_renderers PUBLIC openvkl rkcommon::rkcommon)

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

  ${VKL_RESOURCE}
)

target_link_libraries(vklExamples PRIVATE
  openvkl_vdb_util
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

    ${VKL_RESOURCE}
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

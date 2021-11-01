## Copyright 2019-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_subdirectory(renderer)

### vklExamples app ##

set(OpenGL_GL_PREFERENCE "GLVND") # Request new ABI, but this is only a hint.
find_package(OpenGL 2)

if (OPENGL_FOUND)
  add_library(vkl_opengl INTERFACE)
  target_compile_definitions(vkl_opengl INTERFACE GL_SILENCE_DEPRECATION)
  if (TARGET OpenGL::GL)
    target_link_libraries(vkl_opengl INTERFACE OpenGL::GL)
  else ()
    # Old versions of cmake do not create the GL targets.
    target_link_libraries(vkl_opengl INTERFACE ${OPENGL_LIBRARIES})
    target_include_directories(vkl_opengl INTERFACE ${OPENGL_INCLUDE_DIR})
  endif ()

  find_package(glfw3 REQUIRED)

  set(OPENVKL_IMGUI_ROOT "${CMAKE_CURRENT_LIST_DIR}/imgui-1.83"
    CACHE PATH "Path to imgui.")

  add_library(imgui STATIC
    ${OPENVKL_IMGUI_ROOT}/imgui.cpp
    ${OPENVKL_IMGUI_ROOT}/imgui_draw.cpp
    ${OPENVKL_IMGUI_ROOT}/imgui_tables.cpp
    ${OPENVKL_IMGUI_ROOT}/imgui_widgets.cpp
    ${OPENVKL_IMGUI_ROOT}/backends/imgui_impl_glfw.cpp
    ${OPENVKL_IMGUI_ROOT}/backends/imgui_impl_opengl2.cpp)

  target_include_directories(imgui PUBLIC ${OPENVKL_IMGUI_ROOT})
  target_link_libraries(imgui PUBLIC glfw vkl_opengl)
  target_compile_definitions(imgui PUBLIC VKL_HAVE_IMGUI)
endif()

if (TARGET imgui)
  add_executable(vklExamples
    vklExamples.cpp
    BatchApplication.cpp
    InteractiveApplication.cpp
    ParameterGui.cpp
    RenderView.cpp
    TransferFunctionWidget.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklExamples
    PRIVATE
      imgui
      vkl_example_renderers
  )

  install(TARGETS vklExamples
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

### vklBenchmark app ##

if (BUILD_BENCHMARKS)
  add_executable(vklBenchmark
    vklBenchmark.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmark
  PRIVATE
    benchmark
    openvkl_testing
    vkl_example_renderers
  )

  install(TARGETS vklBenchmark RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

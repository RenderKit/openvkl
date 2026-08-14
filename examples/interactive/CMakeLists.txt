## Copyright 2019 Intel Corporation
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

  add_subdirectory(external/imgui)
endif()

set(ENABLED_TARGETS "CPU")

if (${OPENVKL_ENABLE_DEVICE_GPU})
  list(APPEND ENABLED_TARGETS "GPU")
endif()

if (TARGET imgui)

  # separate target, WA for https://github.com/intel/llvm/issues/22972
  add_library(vkl_example_dockspace STATIC Dockspace.cpp)
  target_link_libraries(vkl_example_dockspace PUBLIC imgui)

  foreach (ET IN LISTS ENABLED_TARGETS)  
    add_executable(vklExamples${ET}
      vklExamples.cpp
      BatchApplication.cpp
      InteractiveApplication.cpp
      ParameterGui.cpp
      RenderView.cpp
      TransferFunctionWidget.cpp
      ${VKL_RESOURCE}
    )

    string(TOLOWER ${ET} ET_LOWER)

    target_link_libraries(vklExamples${ET}
      PRIVATE
        vkl_example_dockspace
        vkl_example_renderers_${ET_LOWER}
    )

    install(TARGETS vklExamples${ET}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endforeach()

endif()

### vklBenchmark app ##

if (BUILD_BENCHMARKS)
  foreach (ET IN LISTS ENABLED_TARGETS)  
    add_executable(vklBenchmark${ET}
      vklBenchmark.cpp
      ${VKL_RESOURCE}
    )

    string(TOLOWER ${ET} ET_LOWER)

    target_link_libraries(vklBenchmark${ET}
    PRIVATE
      benchmark::benchmark
      vkl_example_renderers_${ET_LOWER}
    )

    install(TARGETS vklBenchmark${ET} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endforeach()
endif()

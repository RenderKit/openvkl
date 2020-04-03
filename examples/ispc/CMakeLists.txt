## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_ispc)

macro(add_executable_ispc name)
  set(ISPC_SOURCES "")
  set(OTHER_SOURCES "")
  foreach(src ${ARGN})
    get_filename_component(ext ${src} EXT)
    if (ext STREQUAL ".ispc")
      set(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    else()
      set(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    endif ()
  endforeach()
  openvkl_ispc_compile(${ISPC_SOURCES})
  add_executable(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES} ${VKL_RESOURCE})
endmacro()

include_directories_ispc(${PROJECT_SOURCE_DIR}/openvkl/include)

add_executable_ispc(vklTutorialISPC
  vklTutorialISPC.ispc
  vklTutorialISPC.cpp
)

target_include_directories(vklTutorialISPC PRIVATE ${ISPC_TARGET_DIR})

target_link_libraries(vklTutorialISPC PRIVATE openvkl)

install(TARGETS vklTutorialISPC
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

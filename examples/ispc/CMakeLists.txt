## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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

include_directories_ispc(
  ${PROJECT_SOURCE_DIR}/openvkl/include
  ${RKCOMMON_INCLUDE_DIRS}
)

get_target_property(CPU_DEVICE_INCLUDE_DIRS openvkl_module_cpu_device INTERFACE_INCLUDE_DIRECTORIES)
include_directories_ispc(${CPU_DEVICE_INCLUDE_DIRS})

add_executable_ispc(vklTutorialISPC
  vklTutorialISPC.ispc
  vklTutorialISPC.cpp
)

target_include_directories(vklTutorialISPC PRIVATE ${ISPC_TARGET_DIR})

target_link_libraries(vklTutorialISPC PRIVATE openvkl openvkl_module_cpu_device)

install(TARGETS vklTutorialISPC
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

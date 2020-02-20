## ======================================================================== ##
## Copyright 2019-2020 Intel Corporation                                    ##
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
  add_executable(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})
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

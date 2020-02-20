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

macro(openvkl_add_library_ispc name type)
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
  add_library(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES})
endmacro()

macro(openvkl_add_executable_ispc name)
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

macro(openvkl_configure_build_type)
  set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
  if (WIN32)
    if (NOT OPENVKL_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
      set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
          CACHE STRING "List of generated configurations." FORCE)
      set(OPENVKL_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
          CACHE INTERNAL "Default CMake configuration types set.")
    endif()
  else()
    if(NOT CMAKE_BUILD_TYPE)
      set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
    endif()
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
  endif()
endmacro()

macro(openvkl_create_embree_target)
  if (NOT TARGET embree)
    add_library(embree INTERFACE) # NOTE(jda) - Cannot be IMPORTED due to CMake
                                  #             issues found on Ubuntu.

    target_include_directories(embree
    INTERFACE
      $<BUILD_INTERFACE:${EMBREE_INCLUDE_DIRS}>
    )

    target_link_libraries(embree
    INTERFACE
      $<BUILD_INTERFACE:${EMBREE_LIBRARY}>
    )
  endif()
endmacro()

function(openvkl_get_width_enabled WIDTH ENABLED)
  set(${ENABLED} FALSE PARENT_SCOPE)

  if(WIDTH EQUAL 4 AND DEFINED OPENVKL_ISPC_TARGET_LIST_4)
    set(${ENABLED} TRUE PARENT_SCOPE)
  elseif(WIDTH EQUAL 8 AND DEFINED OPENVKL_ISPC_TARGET_LIST_8)
    set(${ENABLED} TRUE PARENT_SCOPE)
  elseif(WIDTH EQUAL 16 AND DEFINED OPENVKL_ISPC_TARGET_LIST_16)
    set(${ENABLED} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(openvkl_get_compile_options_for_width WIDTH FLAGS)
  set(LOCAL_FLAGS "")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

     message("detected Clang or GNU compiler")

    if(WIDTH EQUAL 4)
      set(LOCAL_FLAGS "-msse4.2")
    elseif(WIDTH EQUAL 8)
      set(LOCAL_FLAGS "-mavx")
    elseif(WIDTH EQUAL 16)
      set(LOCAL_FLAGS "-mavx512f")
    else()
      message(FATAL_ERROR "unknown build width")
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

    message("detected Intel compiler")

    if(WIN32)
      if(WIDTH EQUAL 4)
        set(LOCAL_FLAGS "/QxSSE4.2")
      elseif(WIDTH EQUAL 8)
        set(LOCAL_FLAGS "/arch:AVX")
      elseif(WIDTH EQUAL 16)
        set(LOCAL_FLAGS "/QxCORE-AVX512")
      else()
        message(FATAL_ERROR "unknown build width")
      endif()
    else()
      if(WIDTH EQUAL 4)
        set(LOCAL_FLAGS "-xsse4.2")
      elseif(WIDTH EQUAL 8)
        set(LOCAL_FLAGS "-xAVX")
      elseif(WIDTH EQUAL 16)
        set(LOCAL_FLAGS "-xCORE-AVX512")
      else()
        message(FATAL_ERROR "unknown build width")
      endif()
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    message("detected MSVC compiler")

    if(WIDTH EQUAL 4)
      set(LOCAL_FLAGS "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__")
    elseif(WIDTH EQUAL 8)
      set(LOCAL_FLAGS "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__ /arch:AVX")
    elseif(WIDTH EQUAL 16)
      message(WARNING "only setting AVX compile options for width ${WIDTH} under MSVC")
      set(LOCAL_FLAGS "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__ /arch:AVX")
    else()
      message(FATAL_ERROR "unknown build width")
    endif()

  else()
    message(WARNING "unknown compiler, not setting width-specific build flags")
  endif()

  # verify width-specific build flags work on the current compiler
  try_compile(COMPILER_SUPPORTS_WIDTH_FLAGS
    ${CMAKE_BINARY_DIR}
    ${PROJECT_SOURCE_DIR}/cmake/check_build.cpp
    COMPILE_DEFINITIONS ${LOCAL_FLAGS}
  )

  if(COMPILER_SUPPORTS_WIDTH_FLAGS)
    set(${FLAGS} ${LOCAL_FLAGS} PARENT_SCOPE)
  else()
    message(WARNING "compiler does not support build flags for width ${WIDTH}: ${LOCAL_FLAGS}. Performance may not be optimal.")
    set(${FLAGS} "" PARENT_SCOPE)
  endif()

endfunction()

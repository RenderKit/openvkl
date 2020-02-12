## ======================================================================== ##
## Copyright 2019 Intel Corporation                                         ##
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

  if(WIDTH EQUAL 4 AND OPENVKL_ISA_SSE4)
    set(${ENABLED} TRUE PARENT_SCOPE)
  elseif(WIDTH EQUAL 8 AND (OPENVKL_ISA_AVX OR OPENVKL_ISA_AVX2))
    set(${ENABLED} TRUE PARENT_SCOPE)
  elseif(WIDTH EQUAL 16 AND (OPENVKL_ISA_AVX512KNL OR OPENVKL_ISA_AVX512SKX))
    set(${ENABLED} TRUE PARENT_SCOPE)
  endif()
endfunction()

function(openvkl_get_compile_options_for_width WIDTH FLAGS)
  set(${FLAGS} "" PARENT_SCOPE)

  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

     message("detected Clang or GNU compiler")

    if(WIDTH EQUAL 4)
      set(${FLAGS} "-msse4.2" PARENT_SCOPE)
    elseif(WIDTH EQUAL 8)
      set(${FLAGS} "-mavx" PARENT_SCOPE)
    elseif(WIDTH EQUAL 16)
      set(${FLAGS} "-mavx512f" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "unknown build width")
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

    message("detected Intel compiler")

    if(WIN32)
      if(WIDTH EQUAL 4)
        set(${FLAGS} "/QxSSE4.2" PARENT_SCOPE)
      elseif(WIDTH EQUAL 8)
        set(${FLAGS} "/arch:AVX" PARENT_SCOPE)
      elseif(WIDTH EQUAL 16)
        set(${FLAGS} "/QxCORE-AVX512" PARENT_SCOPE)
      else()
        message(FATAL_ERROR "unknown build width")
      endif()
    else()
      if(WIDTH EQUAL 4)
        set(${FLAGS} "-xsse4.2" PARENT_SCOPE)
      elseif(WIDTH EQUAL 8)
        set(${FLAGS} "-xAVX" PARENT_SCOPE)
      elseif(WIDTH EQUAL 16)
        set(${FLAGS} "-xCORE-AVX512" PARENT_SCOPE)
      else()
        message(FATAL_ERROR "unknown build width")
      endif()
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    message("detected MSVC compiler")

    if(WIDTH EQUAL 4)
      set(${FLAGS} "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__" PARENT_SCOPE)
    elseif(WIDTH EQUAL 8)
      set(${FLAGS} "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__ /arch:AVX" PARENT_SCOPE)
    elseif(WIDTH EQUAL 16)
      message(WARNING "only setting AVX compile options for width ${WIDTH} under MSVC")
      set(${FLAGS} "/D__SSE__ /D__SSE2__ /D__SSE3__ /D__SSSE3__ /D__SSE4_1__ /D__SSE4_2__ /arch:AVX" PARENT_SCOPE)
    else()
      message(FATAL_ERROR "unknown build width")
    endif()

  else()
    message(WARNING "unknown compiler, not setting width-specific build flags")
  endif()

endfunction()

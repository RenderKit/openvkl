## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

option(OPENVKL_ISPC_FAST_MATH "enable ISPC fast-math optimizations" OFF)

set(OPENVKL_ISPC_ADDRESSING 32 CACHE STRING "32 vs 64 bit addressing in ispc")
set_property(CACHE OPENVKL_ISPC_ADDRESSING PROPERTY STRINGS 32 64)
mark_as_advanced(OPENVKL_ISPC_ADDRESSING)

if (NOT (OPENVKL_ISPC_ADDRESSING STREQUAL "32" OR
         OPENVKL_ISPC_ADDRESSING STREQUAL "64"))
  message(FATAL_ERROR "OPENVKL_ISPC_ADDRESSING must be set to either '32' or '64'!")
endif()

macro(openvkl_configure_ispc_isa)

  set(OPENVKL_BUILD_ISA "ALL" CACHE STRING
      "Target ISA (SSE4, AVX, AVX2, AVX512KNL, AVX512SKX, or ALL)")
  string(TOUPPER ${OPENVKL_BUILD_ISA} OPENVKL_BUILD_ISA)

  option(OPENVKL_BUILD_ISA_SCALAR
         "Include 'SCALAR' target (WARNING: may not work!)" OFF)
  mark_as_advanced(OPENVKL_BUILD_ISA_SCALAR)

  if (OPENVKL_BUILD_ISA_SCALAR)
    set(OPENVKL_SUPPORTED_ISAS SCALAR)
  endif()

    set(OPENVKL_SUPPORTED_ISAS ${OPENVKL_SUPPORTED_ISAS} SSE4)
    set(OPENVKL_SUPPORTED_ISAS ${OPENVKL_SUPPORTED_ISAS} AVX)
    set(OPENVKL_SUPPORTED_ISAS ${OPENVKL_SUPPORTED_ISAS} AVX2)
    set(OPENVKL_SUPPORTED_ISAS ${OPENVKL_SUPPORTED_ISAS} AVX512KNL)
    set(OPENVKL_SUPPORTED_ISAS ${OPENVKL_SUPPORTED_ISAS} AVX512SKX)

  set_property(CACHE OPENVKL_BUILD_ISA PROPERTY STRINGS
               ALL ${OPENVKL_SUPPORTED_ISAS})

  unset(OPENVKL_ISPC_TARGET_LIST)

  if (OPENVKL_BUILD_ISA STREQUAL "ALL")
    set(OPENVKL_ISPC_TARGET_LIST ${OPENVKL_ISPC_TARGET_LIST} sse4)
    message(STATUS "OpenVKL SSE4 ISA target enabled.")

    set(OPENVKL_ISPC_TARGET_LIST ${OPENVKL_ISPC_TARGET_LIST} avx)
    message(STATUS "OpenVKL AVX ISA target enabled.")

    set(OPENVKL_ISPC_TARGET_LIST ${OPENVKL_ISPC_TARGET_LIST} avx2)
    message(STATUS "OpenVKL AVX2 ISA target enabled.")

    set(OPENVKL_ISPC_TARGET_LIST ${OPENVKL_ISPC_TARGET_LIST} avx512knl-i32x16)
    message(STATUS "OpenVKL AVX512KNL ISA target enabled.")

    set(OPENVKL_ISPC_TARGET_LIST ${OPENVKL_ISPC_TARGET_LIST} avx512skx-i32x16)
    message(STATUS "OpenVKL AVX512SKX ISA target enabled.")

  elseif (OPENVKL_BUILD_ISA STREQUAL "AVX512SKX")
    set(OPENVKL_ISPC_TARGET_LIST avx512skx-i32x16)

  elseif (OPENVKL_BUILD_ISA STREQUAL "AVX512KNL")
    set(OPENVKL_ISPC_TARGET_LIST avx512knl-i32x16)

  elseif (OPENVKL_BUILD_ISA STREQUAL "AVX2")
    set(OPENVKL_ISPC_TARGET_LIST avx2)

  elseif (OPENVKL_BUILD_ISA STREQUAL "AVX")
    set(OPENVKL_ISPC_TARGET_LIST avx)

  elseif (OPENVKL_BUILD_ISA STREQUAL "SSE4")
    set(OPENVKL_ISPC_TARGET_LIST sse4)

  else()
    message(ERROR "Invalid OPENVKL_BUILD_ISA value. "
                  "Please select one of ${OPENVKL_SUPPORTED_ISAS}, or ALL.")
  endif()
endmacro()

macro (OPENVKL_ISPC_COMPILE)
  set(ISPC_ADDITIONAL_ARGS "")
  set(ISPC_TARGETS ${OPENVKL_ISPC_TARGET_LIST})

  set(ISPC_TARGET_EXT ${CMAKE_CXX_OUTPUT_EXTENSION})
  string(REPLACE ";" "," ISPC_TARGET_ARGS "${ISPC_TARGETS}")

  if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ISPC_ARCHITECTURE "x86-64")
  else()
    set(ISPC_ARCHITECTURE "x86")
  endif()

  set(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
  include_directories(${ISPC_TARGET_DIR})

  if(ISPC_INCLUDE_DIR)
    string(REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
    set(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS})
  endif()

  #CAUTION: -O0/1 -g with ispc seg faults
  set(ISPC_FLAGS_DEBUG "-g" CACHE STRING "ISPC Debug flags")
  mark_as_advanced(ISPC_FLAGS_DEBUG)
  set(ISPC_FLAGS_RELEASE "-O3" CACHE STRING "ISPC Release flags")
  mark_as_advanced(ISPC_FLAGS_RELEASE)
  set(ISPC_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "ISPC Release with Debug symbols flags")
  mark_as_advanced(ISPC_FLAGS_RELWITHDEBINFO)
  if (WIN32 OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELEASE})
  elseif ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_DEBUG})
  else()
    set(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELWITHDEBINFO})
  endif()

  # turn space sparated list into ';' separated list
  string(REPLACE " " ";" ISPC_OPT_FLAGS "${ISPC_OPT_FLAGS}")

  if (NOT WIN32)
    set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --pic)
  endif()

  if (NOT OPENVKL_DEBUG_BUILD)
    set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --opt=disable-assertions)
  endif()

  if (OPENVKL_ISPC_FAST_MATH)
    set(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --opt=fast-math)
  endif()

  set(ISPC_OBJECTS "")

  foreach(src ${ARGN})
    get_filename_component(fname ${src} NAME_WE)
    get_filename_component(dir ${src} PATH)

    set(input ${src})
    if ("${dir}" MATCHES "^/") # absolute unix-style path to input
      set(outdir "${ISPC_TARGET_DIR}/rebased${dir}")
    elseif ("${dir}" MATCHES "^[A-Z]:") # absolute DOS-style path to input
      string(REGEX REPLACE "^[A-Z]:" "${ISPC_TARGET_DIR}/rebased/" outdir "${dir}")
    else() # relative path to input
      set(outdir "${ISPC_TARGET_DIR}/local_${OPENVKL_ISPC_TARGET_NAME}_${dir}")
      set(input ${CMAKE_CURRENT_SOURCE_DIR}/${src})
    endif()

    set(deps "")
    if (EXISTS ${outdir}/${fname}.dev.idep)
      file(READ ${outdir}/${fname}.dev.idep contents)
      string(REPLACE " " ";"     contents "${contents}")
      string(REPLACE ";" "\\\\;" contents "${contents}")
      string(REPLACE "\n" ";"    contents "${contents}")
      foreach(dep ${contents})
        if (EXISTS ${dep})
          set(deps ${deps} ${dep})
        endif (EXISTS ${dep})
      endforeach(dep ${contents})
    endiF ()

    set(results "${outdir}/${fname}.dev${ISPC_TARGET_EXT}")
    # if we have multiple targets add additional object files
    list(LENGTH ISPC_TARGETS NUM_TARGETS)
    if (NUM_TARGETS GREATER 1)
      foreach(target ${ISPC_TARGETS})
        string(REPLACE "-i32x16" "" target ${target}) # strip avx512(knl|skx)-i32x16
        set(results ${results} "${outdir}/${fname}.dev_${target}${ISPC_TARGET_EXT}")
      endforeach()
    endif()

    set(ISPC_FAST_MATH_ARGUMENTS)
    add_custom_command(
      OUTPUT ${results} ${ISPC_TARGET_DIR}/${fname}_ispc.h
      COMMAND ${CMAKE_COMMAND} -E make_directory ${outdir}
      COMMAND ${ISPC_EXECUTABLE}
      ${ISPC_DEFINITIONS}
      -I ${CMAKE_CURRENT_SOURCE_DIR}
      ${ISPC_INCLUDE_DIR_PARMS}
      --arch=${ISPC_ARCHITECTURE}
      --addressing=${OPENVKL_ISPC_ADDRESSING}
      ${ISPC_OPT_FLAGS}
      --target=${ISPC_TARGET_ARGS}
      --woff
      ${ISPC_ADDITIONAL_ARGS}
      -h ${ISPC_TARGET_DIR}/${fname}_ispc.h
      -MMM  ${outdir}/${fname}.dev.idep
      -o ${outdir}/${fname}.dev${ISPC_TARGET_EXT}
      ${input}
      DEPENDS ${input} ${deps}
      COMMENT "Building ISPC object ${outdir}/${fname}.dev${ISPC_TARGET_EXT}"
    )

    set(ISPC_OBJECTS ${ISPC_OBJECTS} ${results})
  endforeach()
endmacro()

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

OPTION(OPENVKL_ISPC_FAST_MATH "enable ISPC fast-math optimizations" OFF)

SET(OPENVKL_ISPC_ADDRESSING 32 CACHE INT "32 vs 64 bit addressing in ispc")
SET_PROPERTY(CACHE OPENVKL_ISPC_ADDRESSING PROPERTY STRINGS 32 64)
MARK_AS_ADVANCED(OPENVKL_ISPC_ADDRESSING)

IF (NOT (OPENVKL_ISPC_ADDRESSING STREQUAL "32" OR
         OPENVKL_ISPC_ADDRESSING STREQUAL "64"))
  MESSAGE(FATAL_ERROR "OPENVKL_ISPC_ADDRESSING must be set to either '32' or '64'!")
ENDIF()


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


MACRO (OPENVKL_ISPC_COMPILE)
  SET(ISPC_ADDITIONAL_ARGS "")
  SET(ISPC_TARGETS ${OPENVKL_ISPC_TARGET_LIST})

  SET(ISPC_TARGET_EXT ${CMAKE_CXX_OUTPUT_EXTENSION})
  STRING(REPLACE ";" "," ISPC_TARGET_ARGS "${ISPC_TARGETS}")

  IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
    SET(ISPC_ARCHITECTURE "x86-64")
  ELSE()
    SET(ISPC_ARCHITECTURE "x86")
  ENDIF()

  SET(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})
  INCLUDE_DIRECTORIES(${ISPC_TARGET_DIR})

  IF(ISPC_INCLUDE_DIR)
    STRING(REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
    SET(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS})
  ENDIF()

  #CAUTION: -O0/1 -g with ispc seg faults
  SET(ISPC_FLAGS_DEBUG "-g" CACHE STRING "ISPC Debug flags")
  MARK_AS_ADVANCED(ISPC_FLAGS_DEBUG)
  SET(ISPC_FLAGS_RELEASE "-O3" CACHE STRING "ISPC Release flags")
  MARK_AS_ADVANCED(ISPC_FLAGS_RELEASE)
  SET(ISPC_FLAGS_RELWITHDEBINFO "-O2 -g" CACHE STRING "ISPC Release with Debug symbols flags")
  MARK_AS_ADVANCED(ISPC_FLAGS_RELWITHDEBINFO)
  IF (WIN32 OR "${CMAKE_BUILD_TYPE}" STREQUAL "Release")
    SET(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELEASE})
  ELSEIF ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    SET(ISPC_OPT_FLAGS ${ISPC_FLAGS_DEBUG})
  ELSE()
    SET(ISPC_OPT_FLAGS ${ISPC_FLAGS_RELWITHDEBINFO})
  ENDIF()

  # turn space sparated list into ';' separated list
  STRING(REPLACE " " ";" ISPC_OPT_FLAGS "${ISPC_OPT_FLAGS}")

  IF (NOT WIN32)
    SET(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --pic)
  ENDIF()

  IF (NOT OPENVKL_DEBUG_BUILD)
    SET(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --opt=disable-assertions)
  ENDIF()

  IF (OPENVKL_ISPC_FAST_MATH)
    SET(ISPC_ADDITIONAL_ARGS ${ISPC_ADDITIONAL_ARGS} --opt=fast-math)
  ENDIF()

  SET(ISPC_OBJECTS "")

  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(fname ${src} NAME_WE)
    GET_FILENAME_COMPONENT(dir ${src} PATH)

    SET(input ${src})
    IF ("${dir}" MATCHES "^/") # absolute unix-style path to input
      SET(outdir "${ISPC_TARGET_DIR}/rebased${dir}")
    ELSEIF ("${dir}" MATCHES "^[A-Z]:") # absolute DOS-style path to input
      STRING(REGEX REPLACE "^[A-Z]:" "${ISPC_TARGET_DIR}/rebased/" outdir "${dir}")
    ELSE() # relative path to input
      SET(outdir "${ISPC_TARGET_DIR}/local_${OPENVKL_ISPC_TARGET_NAME}_${dir}")
      SET(input ${CMAKE_CURRENT_SOURCE_DIR}/${src})
    ENDIF()

    SET(deps "")
    IF (EXISTS ${outdir}/${fname}.dev.idep)
      FILE(READ ${outdir}/${fname}.dev.idep contents)
      STRING(REPLACE " " ";"     contents "${contents}")
      STRING(REPLACE ";" "\\\\;" contents "${contents}")
      STRING(REPLACE "\n" ";"    contents "${contents}")
      FOREACH(dep ${contents})
        IF (EXISTS ${dep})
          SET(deps ${deps} ${dep})
        ENDIF (EXISTS ${dep})
      ENDFOREACH(dep ${contents})
    ENDIF ()

    SET(results "${outdir}/${fname}.dev${ISPC_TARGET_EXT}")
    # if we have multiple targets add additional object files
    LIST(LENGTH ISPC_TARGETS NUM_TARGETS)
    IF (NUM_TARGETS GREATER 1)
      FOREACH(target ${ISPC_TARGETS})
        STRING(REPLACE "-i32x16" "" target ${target}) # strip avx512(knl|skx)-i32x16
        SET(results ${results} "${outdir}/${fname}.dev_${target}${ISPC_TARGET_EXT}")
      ENDFOREACH()
    ENDIF()

    SET(ISPC_FAST_MATH_ARGUMENTS)
    ADD_CUSTOM_COMMAND(
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

    SET(ISPC_OBJECTS ${ISPC_OBJECTS} ${results})
  ENDFOREACH()
ENDMACRO()

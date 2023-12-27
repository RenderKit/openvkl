## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

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
  set(ISPC_TARGET_NAME ${name})
  openvkl_ispc_compile(${ISPC_SOURCES})
  unset(ISPC_TARGET_NAME)
  add_library(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES} ${VKL_RESOURCE})
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
  set(ISPC_TARGET_NAME ${name})
  openvkl_ispc_compile(${ISPC_SOURCES})
  unset(ISPC_TARGET_NAME)
  add_executable(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES} ${ISPC_SOURCES} ${VKL_RESOURCE})
  target_include_directories(${name} PRIVATE "${COMMON_PATH}" "${LEVEL_ZERO_INCLUDE_DIR}" "${ISPC_INCLUDE_DIR}" )
  target_link_libraries(${name} PRIVATE ${LEVEL_ZERO_LIB_LOADER})
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

macro(openvkl_configure_global_build_flags)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")

     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

     if(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
      if (WIN32)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise")
      else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fp-model=precise")
      endif()
     endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    if(NOT WIN32)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-ansi-alias")
    endif()
  endif()

  if(WIN32)
    # set DEPENDENTLOADFLAG:LOAD_LIBRARY_SAFE_CURRENT_DIRS on all binaries
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DEPENDENTLOADFLAG:0x2000")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DEPENDENTLOADFLAG:0x2000")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /Qoption,link,/DEPENDENTLOADFLAG:0x2000")
    else()
      message(WARNING "Unrecognized compiler, DEPENDENTLOADFLAG can't be set")
    endif()
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
     CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
     CMAKE_CXX_COMPILER_ID STREQUAL "IntelLLVM")

    message(STATUS "detected Clang or GNU compiler")

    if(NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm64|aarch64")
      if(WIDTH EQUAL 4)
        set(LOCAL_FLAGS "-msse4.2")
      elseif(WIDTH EQUAL 8)
        set(LOCAL_FLAGS "-mavx")
      elseif(WIDTH EQUAL 16)
        set(LOCAL_FLAGS "-mavx512f")
      else()
        message(FATAL_ERROR "unknown build width")
      endif()
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")

    message(STATUS "detected Intel compiler")

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

      set(LOCAL_FLAGS "${LOCAL_FLAGS} -no-inline-max-total-size")   # no size limit when performing inlining
      set(LOCAL_FLAGS "${LOCAL_FLAGS} -no-inline-max-per-compile")  # no maximal number of inlinings per compilation unit
      set(LOCAL_FLAGS "${LOCAL_FLAGS} -inline-factor=150")          # increase default inline factors limits by 2x
    endif()

  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")

    message(STATUS "detected MSVC compiler")

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

macro(openvkl_install_library name)
  set_target_properties(${name}
    PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
  openvkl_install_target(${name})
endmacro()

macro(openvkl_install_target name)
  install(TARGETS ${name}
    EXPORT ${PROJECT_NAME}_Exports
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      NAMELINK_SKIP
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  install(EXPORT ${PROJECT_NAME}_Exports
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}-${PROJECT_VERSION}
    NAMESPACE openvkl::
  )

  install(TARGETS ${name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      NAMELINK_ONLY
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
endmacro()

# Generate files that deal with the VDB topology.
# There are "templatized" files for traversal, for example,
# and we also define constants that describe the topology
# in terms of node resolution per level.
function(openvkl_vdb_generate_topology)
  set(VKL_VDB_NUM_LEVELS "4")

  set(VKL_VDB_LOG_RESOLUTION "")
  list(APPEND VKL_VDB_LOG_RESOLUTION "${VKL_VDB_LOG_RESOLUTION_3}")
  list(APPEND VKL_VDB_LOG_RESOLUTION "${VKL_VDB_LOG_RESOLUTION_2}")
  list(APPEND VKL_VDB_LOG_RESOLUTION "${VKL_VDB_LOG_RESOLUTION_1}")
  list(APPEND VKL_VDB_LOG_RESOLUTION "${VKL_VDB_LOG_RESOLUTION_0}")

  math(EXPR VKL_VDB_LEAF_LEVEL "${VKL_VDB_NUM_LEVELS}-1")
  set(VKL_VDB_TOTAL_LOG_RES 0)

  foreach(I RANGE ${VKL_VDB_LEAF_LEVEL})
    math(EXPR VKL_VDB_LEVEL "${VKL_VDB_LEAF_LEVEL}-${I}")
    math(EXPR VKL_VDB_NEXT_LEVEL "${VKL_VDB_LEVEL}+1")

    list(GET VKL_VDB_LOG_RESOLUTION ${I} VKL_VDB_LEVEL_LOG_RES)
    math(EXPR VKL_VDB_LEVEL_STORAGE_RES "(1<<${VKL_VDB_LEVEL_LOG_RES})")
    math(EXPR VKL_VDB_LEVEL_NUM_VOXELS "(1<<(3*${VKL_VDB_LEVEL_LOG_RES}))")
    math(EXPR VKL_VDB_TOTAL_LOG_RES "(${VKL_VDB_TOTAL_LOG_RES}+${VKL_VDB_LEVEL_LOG_RES})")
    math(EXPR VKL_VDB_LEVEL_RES "(1<<${VKL_VDB_TOTAL_LOG_RES})")

    set(VKL_VDB_POSTFIX "")
    if (${VKL_VDB_LEVEL} GREATER 0)
      set(VKL_VDB_POSTFIX "_${VKL_VDB_LEVEL}")
    endif()

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}/vdb_topology.h.in
      include/${PROJECT_NAME}/vdb/topology${VKL_VDB_POSTFIX}.h
    )

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/VdbSamplerDispatchInner.ih.in
      include/${PROJECT_NAME}_vdb/VdbSamplerDispatchInner${VKL_VDB_POSTFIX}.ih
    )

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/gpu/compute/vdb/VdbSamplerDispatchInner.h.in
      include/${PROJECT_NAME}_vdb/VdbSamplerDispatchInner${VKL_VDB_POSTFIX}.h
    )

    # Generate uniform, varying, and univary traversal.
    # a) We know all queries are in the same leaf node. Fully uniform traversal.
    set(VKL_VDB_UNIVARY_IN "uniform")
    set(VKL_VDB_UNIVARY_OUT "uniform")
    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/VdbSampleInner.ih.in
      include/${PROJECT_NAME}_vdb/VdbSampleInner_${VKL_VDB_UNIVARY_IN}_${VKL_VDB_UNIVARY_OUT}_${VKL_VDB_LEVEL}.ih
    )

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/gpu/compute/vdb/VdbSampleInner.h.in
      include/${PROJECT_NAME}_vdb/VdbSampleInner_${VKL_VDB_UNIVARY_IN}_${VKL_VDB_UNIVARY_OUT}_${VKL_VDB_LEVEL}.h
    )
    # b) All lanes are not in the same subtree.
    set(VKL_VDB_UNIVARY_IN "varying")
    set(VKL_VDB_UNIVARY_OUT "varying")
    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/VdbSampleInner.ih.in
      include/${PROJECT_NAME}_vdb/VdbSampleInner_${VKL_VDB_UNIVARY_IN}_${VKL_VDB_UNIVARY_OUT}_${VKL_VDB_LEVEL}.ih
    )
    # c) Lanes are not in the same leaf, but currently in the same subtree.
    #    May degenerate to varying.
    set(VKL_VDB_UNIVARY_IN "uniform")
    set(VKL_VDB_UNIVARY_OUT "varying")
    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/VdbSampleInner.ih.in
      include/${PROJECT_NAME}_vdb/VdbSampleInner_${VKL_VDB_UNIVARY_IN}_${VKL_VDB_UNIVARY_OUT}_${VKL_VDB_LEVEL}.ih
    )

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/VdbQueryVoxel.ih.in
      include/${PROJECT_NAME}_vdb/VdbQueryVoxel_${VKL_VDB_LEVEL}.ih
    )

    configure_file(
      ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/devices/cpu/volume/vdb/HDDA.ih.in
      include/${PROJECT_NAME}_vdb/HDDA_${VKL_VDB_LEVEL}.ih
    )

  endforeach(I)

endfunction()


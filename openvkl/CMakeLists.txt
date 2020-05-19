## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include(openvkl_ispc)

if(WIN32)
  set(DEF_FILE common/ispc_defs.def)
endif()

set(VKL_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/include/openvkl/openvkl.rc")
set(VKL_RESOURCE ${VKL_RESOURCE} PARENT_SCOPE)

openvkl_add_library_ispc(${PROJECT_NAME} SHARED
  api/API.cpp
  api/Driver.cpp

  common/Data.cpp
  common/ispc_util.ispc
  common/logging.cpp
  common/ManagedObject.cpp
  common/Observer.cpp
  common/VKLCommon.cpp

  ${DEF_FILE}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${ISPC_TARGET_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} PRIVATE rkcommon::rkcommon)


set(VKL_VDB_LOG_RESOLUTION "6;5;4;3" CACHE STRING
  "Base-2 logarithm of the resolution for each level in the tree.")
openvkl_vdb_generate_topology()


## Configure OpenVKL installation ##

openvkl_install_library(${PROJECT_NAME})

install(DIRECTORY
  ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
  PATTERN "*.isph"
)

install(DIRECTORY
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}/include/${PROJECT_NAME}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
  PATTERN "*.h"
)

install(FILES
  ${PROJECT_SOURCE_DIR}/LICENSE.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-Embree.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-OSPRay.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-TBB.txt
  ${PROJECT_SOURCE_DIR}/README.md
  ${PROJECT_SOURCE_DIR}/CHANGELOG.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR})

## Version header ##

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/include/${PROJECT_NAME}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/version.h
  @ONLY
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}>
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME}/version.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

## Add OpenVKL drivers ##

add_subdirectory(drivers)

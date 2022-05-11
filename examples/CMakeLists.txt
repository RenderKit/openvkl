## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## "Hello world" VKL tutorials ##

add_executable(vklTutorial vklTutorial.c ${VKL_RESOURCE})
target_link_libraries(vklTutorial PRIVATE openvkl)
install(TARGETS vklTutorial RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

## ISPC tutorial ##

add_subdirectory(ispc)

## Interacive Examples ##

add_subdirectory(interactive)

## Minimal Console-based Examples ##

add_subdirectory(minimal)

## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## "Hello world" VKL tutorials ##

add_executable(vklTutorialCPU vklTutorialCPU.c ${VKL_RESOURCE})
target_link_libraries(vklTutorialCPU PRIVATE openvkl openvkl_module_cpu_device)
install(TARGETS vklTutorialCPU RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if (${OPENVKL_ENABLE_DEVICE_GPU})
  add_executable(vklTutorialGPU vklTutorialGPU.cpp ${VKL_RESOURCE})
  target_link_libraries(vklTutorialGPU PRIVATE openvkl openvkl_module_gpu_device)
  install(TARGETS vklTutorialGPU RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

## ISPC tutorial ##

add_subdirectory(ispc)

## Interacive Examples ##

add_subdirectory(interactive)

## Minimal Console-based Examples ##

add_subdirectory(minimal)

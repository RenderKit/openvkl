## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# The minimal_01 ... minimal_06 examples gradually increase in complexity
foreach(i 01 02 03 04 05 06)
  add_executable(vklMinimal_${i} minimal_${i}.cpp ${VKL_RESOURCE})
  target_link_libraries(vklMinimal_${i} PRIVATE openvkl openvkl_module_cpu_device)
  install(TARGETS vklMinimal_${i} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endforeach()

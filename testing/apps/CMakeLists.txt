## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Benchmarks
if (BUILD_BENCHMARKS)
  # Structured volumes
  add_executable(vklBenchmarkStructuredVolume
    vklBenchmarkStructuredVolume.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkStructuredVolume
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkStructuredVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Structured volumes (multi-attribute)
  add_executable(vklBenchmarkStructuredVolumeMulti
    vklBenchmarkStructuredVolumeMulti.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkStructuredVolumeMulti
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkStructuredVolumeMulti
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Unstructured volumes
  add_executable(vklBenchmarkUnstructuredVolume
    vklBenchmarkUnstructuredVolume.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkUnstructuredVolume
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkUnstructuredVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # VDB volumes
  add_executable(vklBenchmarkVdbVolume
    vklBenchmarkVdbVolume.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkVdbVolume
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkVdbVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # VDBvolumes (multi-attribute)
    add_executable(vklBenchmarkVdbVolumeMulti
    vklBenchmarkVdbVolumeMulti.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkVdbVolumeMulti
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkVdbVolumeMulti
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  # Particle volumes
  add_executable(vklBenchmarkParticleVolume
    vklBenchmarkParticleVolume.cpp
    ${VKL_RESOURCE}
  )

  target_link_libraries(vklBenchmarkParticleVolume
    benchmark
    openvkl_testing_cpu
  )

  install(TARGETS vklBenchmarkParticleVolume
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()

# Functional tests
if (BUILD_TESTING)

  # reset includes used for CPU tests
  set(ISPC_INCLUDE_DIR "")

  include_directories_ispc(
    ${PROJECT_SOURCE_DIR}/openvkl/include
    ${PROJECT_BINARY_DIR}/openvkl/include/${PROJECT_NAME}
    ${PROJECT_SOURCE_DIR}/openvkl/devices/cpu
    ${PROJECT_SOURCE_DIR}/openvkl/devices/cpu/include
    ${RKCOMMON_INCLUDE_DIRS}
  )

  openvkl_add_executable_ispc(vklTestsCPU
    vklTests.cpp
    tests/alignment.cpp
    tests/background_undefined.cpp
    tests/hit_iterator.cpp
    tests/hit_iterator_epsilon.cpp
    tests/interval_iterator.cpp
    tests/simd_conformance.cpp
    tests/simd_conformance.ispc
    tests/simd_type_conversion.cpp
    tests/structured_volume_gradients.cpp
    tests/structured_regular_volume_sampling.cpp
    tests/structured_regular_volume_sampling_motion_blur.cpp
    tests/structured_regular_volume_gradients_motion_blur.cpp
    tests/structured_regular_volume_strides.cpp
    tests/structured_regular_volume_multi.cpp
    tests/structured_spherical_volume_sampling.cpp
    tests/structured_spherical_volume_bounding_box.cpp
    tests/structured_volume_value_range.cpp
    tests/unstructured_volume_gradients.cpp
    tests/unstructured_volume_sampling.cpp
    tests/unstructured_volume_strides.cpp
    tests/unstructured_volume_value_range.cpp
    tests/vectorized_gradients.cpp
    tests/stream_gradients.cpp
    tests/vectorized_hit_iterator.cpp
    tests/vectorized_interval_iterator.cpp
    tests/vectorized_sampling.cpp
    tests/stream_sampling.cpp
    tests/amr_volume_sampling.cpp
    tests/amr_volume_value_range.cpp
    tests/vdb_volume.cpp
    tests/vdb_volume_multi.cpp
    tests/vdb_volume_motion_blur.cpp
    tests/vdb_volume_inner_node_observer.cpp
    tests/vdb_volume_dense.cpp
    tests/particle_volume_sampling.cpp
    tests/particle_volume_gradients.cpp
    tests/particle_volume_value_range.cpp
    tests/particle_volume_radius.cpp
    tests/particle_volume_interval_iterator.cpp
    tests/multi_device.cpp
  )

  target_include_directories(vklTestsCPU PRIVATE ${ISPC_TARGET_DIR})

  target_link_libraries(vklTestsCPU PRIVATE openvkl_testing_cpu)

  # Needed for SIMD conformance tests
  foreach(TARGET_NAME "openvkl_module_cpu_device_4"
                      "openvkl_module_cpu_device_8"
                      "openvkl_module_cpu_device_16")
    if (TARGET ${TARGET_NAME})
      target_link_libraries(vklTestsCPU PRIVATE ${TARGET_NAME})
    endif()
  endforeach()

  install(TARGETS vklTestsCPU
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if (${OPENVKL_ENABLE_DEVICE_GPU})

    # reset includes used for CPU tests
    set(ISPC_INCLUDE_DIR "")

    include_directories_ispc(
      ${PROJECT_SOURCE_DIR}/openvkl/include
      ${PROJECT_SOURCE_DIR}/openvkl/devices/gpu/include
      ${RKCOMMON_INCLUDE_DIRS}
    )

    openvkl_add_executable_ispc(vklTestsGPU
      vklTests.cpp
      tests/structured_regular_volume_sampling.cpp
      tests/structured_regular_volume_multi.cpp
      tests/structured_spherical_volume_sampling.cpp
      tests/structured_spherical_volume_bounding_box.cpp
      tests/structured_volume_gradients.cpp
      tests/structured_volume_value_range.cpp
      tests/unstructured_volume_gradients.cpp
      tests/unstructured_volume_sampling.cpp
      tests/hit_iterator.cpp
      tests/interval_iterator.cpp
      tests/multi_device.cpp
      tests/particle_volume_gradients.cpp
      tests/particle_volume_interval_iterator.cpp
      tests/particle_volume_radius.cpp
      tests/particle_volume_sampling.cpp
      tests/particle_volume_value_range.cpp
    )
    target_include_directories(vklTestsGPU PRIVATE ${ISPC_TARGET_DIR})

    target_link_libraries(vklTestsGPU PRIVATE openvkl_testing_gpu)

    # We need to disable catching signals to make USM Shared buffers on GPU work properly
    target_compile_definitions(vklTestsGPU PRIVATE CATCH_CONFIG_NO_POSIX_SIGNALS=1)

    install(TARGETS vklTestsGPU
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
  endif()
endif()

## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include_directories_ispc(
  ${CMAKE_SOURCE_DIR}/openvkl/include
  ${CMAKE_SOURCE_DIR}/openvkl/devices/cpu/math
  ${RKCOMMON_INCLUDE_DIRS}
)


set(VKL_EXAMPLE_RENDERERS_SRC_CPU
  density_path_tracer/DensityPathTracer.cpp
  density_path_tracer/DensityPathTracerIspc.cpp
  density_path_tracer/DensityPathTracer.ispc
  ArcballCamera.cpp
  framebuffer/BufferCpu.cpp
  framebuffer/Framebuffer.ispc
  IntervalIteratorDebug.cpp
  IntervalIteratorDebug.ispc
  Renderer.ispc
  HitIteratorRenderer.cpp
  HitIteratorRenderer.ispc
  RayMarchIteratorRenderer.cpp
  RayMarchIteratorRenderer.ispc
  Renderer.cpp
  RendererHost.cpp
  RendererParams.cpp
  SamplerParams.cpp
  Scheduler.cpp
  Scene.cpp
  Scene.ispc
  VolumeParams.cpp
)

get_target_property(CPU_DEVICE_INCLUDE_DIRS openvkl_module_cpu_device INTERFACE_INCLUDE_DIRECTORIES)
include_directories_ispc(${CPU_DEVICE_INCLUDE_DIRS})

openvkl_add_library_ispc(vkl_example_renderers_cpu STATIC
  ${VKL_EXAMPLE_RENDERERS_SRC_CPU}
)
target_include_directories(vkl_example_renderers_cpu PUBLIC ${ISPC_TARGET_DIR})
target_link_libraries(vkl_example_renderers_cpu PUBLIC openvkl openvkl_testing_cpu rkcommon::rkcommon)


if (${OPENVKL_ENABLE_DEVICE_GPU})
  set(VKL_EXAMPLE_RENDERERS_SRC_GPU
    density_path_tracer/DensityPathTracerGpu.cpp
    density_path_tracer/DensityPathTracerGpuKernel.cpp
    framebuffer/BufferGpu.cpp
    ArcballCamera.cpp
    RendererGpu.cpp
    Renderer.cpp
    RendererParams.cpp
    SamplerParams.cpp
    Scheduler.cpp
    Scene.cpp
    VolumeParams.cpp
  )

  get_target_property(GPU_DEVICE_INCLUDE_DIRS openvkl_module_gpu_device INTERFACE_INCLUDE_DIRECTORIES)
  include_directories_ispc(${GPU_DEVICE_INCLUDE_DIRS})

  openvkl_add_library_ispc(vkl_example_renderers_gpu STATIC
    ${VKL_EXAMPLE_RENDERERS_SRC_GPU}
  )
  target_include_directories(vkl_example_renderers_gpu PUBLIC ${ISPC_TARGET_DIR})
  target_link_libraries(vkl_example_renderers_gpu PUBLIC openvkl openvkl_testing_gpu rkcommon::rkcommon)
endif()
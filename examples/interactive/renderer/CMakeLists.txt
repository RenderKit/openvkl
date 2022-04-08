## Copyright 2019 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

include_directories_ispc(
  ${CMAKE_SOURCE_DIR}/openvkl/include
  ${CMAKE_SOURCE_DIR}/openvkl/devices/cpu/math
  ${RKCOMMON_INCLUDE_DIRS}
)


set(VKL_EXAMPLE_RENDERERS_SRC
  ArcballCamera.cpp
  DensityPathTracer.cpp
  DensityPathTracer.ispc
  Framebuffer.cpp
  Framebuffer.ispc
  IntervalIteratorDebug.cpp
  IntervalIteratorDebug.ispc
  Renderer.ispc
  HitIteratorRenderer.cpp
  HitIteratorRenderer.ispc
  RayMarchIteratorRenderer.cpp
  RayMarchIteratorRenderer.ispc
  Renderer.cpp
  RendererParams.cpp
  SamplerParams.cpp
  Scheduler.cpp
  Scene.cpp
  Scene.ispc
  VolumeParams.cpp
)

openvkl_add_library_ispc(vkl_example_renderers STATIC
  ${VKL_EXAMPLE_RENDERERS_SRC}
)
target_include_directories(vkl_example_renderers PUBLIC ${ISPC_TARGET_DIR})
target_link_libraries(vkl_example_renderers PUBLIC openvkl openvkl_testing rkcommon::rkcommon)

// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "temporal_data_interpolation.h"

// ---------------------------------------------------------------------------
// Value range.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_computeValueRange_tile_constant(voxelType) \
  inline box1f VdbSampler_computeValueRange_tile_constant_##voxelType(       \
      const VdbGrid *grid,                                                   \
      uint64 /*leafIndex*/,                                                  \
      uint64 leafDataIndex,                                                  \
      const vec2ui & /*xRange*/,                                             \
      const vec2ui & /*yRange*/,                                             \
      const vec2ui & /*zRange*/)                                             \
  {                                                                          \
    const float v = get_##voxelType##_0(grid->leafData[leafDataIndex]);      \
    return make_box1f(v, v);                                                 \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_tile_constant(half)
__vkl_template_VdbSampler_computeValueRange_tile_constant(float)

#undef __vkl_template_VdbSampler_computeValueRange_tile_constant

#define __vkl_template_VdbSampler_computeValueRange_tile_structured(voxelType) \
  inline box1f VdbSampler_computeValueRange_tile_structured_##voxelType(       \
      const VdbGrid *grid,                                                     \
      uint64 leafIndex,                                                        \
      uint64 leafDataIndex,                                                    \
      const vec2ui & /*xRange*/,                                               \
      const vec2ui & /*yRange*/,                                               \
      const vec2ui & /*zRange*/)                                               \
  {                                                                            \
    const uint32 numTimesteps = grid->leafStructuredTimesteps[leafIndex];      \
    box1f valueRange          = make_box1f(pos_inf, neg_inf);                  \
    for (uint32 i = 0; i < numTimesteps; ++i) {                                \
      extend(valueRange, get_##voxelType(grid->leafData[leafDataIndex], i));   \
    }                                                                          \
    return valueRange;                                                         \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_tile_structured(half)
__vkl_template_VdbSampler_computeValueRange_tile_structured(float)

#undef __vkl_template_VdbSampler_computeValueRange_tile_structured

#define __vkl_template_VdbSampler_computeValueRange_tile_unstructured(     \
    voxelType)                                                             \
  inline box1f VdbSampler_computeValueRange_tile_unstructured_##voxelType( \
      const VdbGrid *grid,                                                 \
      uint64 leafIndex,                                                    \
      uint64 leafDataIndex,                                                \
      const vec2ui & /*xRange*/,                                           \
      const vec2ui & /*yRange*/,                                           \
      const vec2ui & /*zRange*/)                                           \
  {                                                                        \
    return computeValueRangeTemporallyUnstructured_##voxelType(            \
        grid->leafData + leafDataIndex,                                    \
        grid->leafUnstructuredIndices + leafIndex,                         \
        0u);                                                               \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_tile_unstructured(half)
__vkl_template_VdbSampler_computeValueRange_tile_unstructured(float)

#undef __vkl_template_VdbSampler_computeValueRange_tile_unstructured

// ---------------------------------------------------------------------------
// Constant leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_tile_constant(voxelType)           \
  inline float VdbSampler_sample_uniform_uniform_tile_constant_##voxelType( \
      const VdbGrid *grid,                                                  \
      vkl_uint64 /*leafIndex*/,                                             \
      vkl_uint64 leafDataIndex,                                             \
      const vec3ui & /*offset*/,                                            \
      float /*time*/)                                                       \
  {                                                                         \
    return get_##voxelType##_0(grid->leafData[leafDataIndex]);              \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_tile_constant(half)
__vkl_template_VdbSampler_sample_tile_constant(float)

#undef __vkl_template_VdbSampler_sample_tile_constant

// ---------------------------------------------------------------------------
// Structured leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_tile_structured(voxelType)           \
  inline float VdbSampler_sample_uniform_uniform_tile_structured_##voxelType( \
      const VdbGrid *grid,                                                    \
      vkl_uint64 leafIndex,                                                   \
      vkl_uint64 leafDataIndex,                                               \
      const vec3ui & /*offset*/,                                              \
      float time)                                                             \
  {                                                                           \
    return interpolateTemporallyStructured_##voxelType(                       \
        grid->leafData + leafDataIndex,                                       \
        grid->leafStructuredTimesteps[leafIndex],                             \
        0u,                                                                   \
        time);                                                                \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_tile_structured(half)
__vkl_template_VdbSampler_sample_tile_structured(float)

#undef __vkl_template_VdbSampler_sample_tile_structured

// ---------------------------------------------------------------------------
// Unstructured leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_tile_unstructured(voxelType)  \
  inline float                                                         \
      VdbSampler_sample_uniform_uniform_tile_unstructured_##voxelType( \
          const VdbGrid *grid,                                         \
          vkl_uint64 leafIndex,                                        \
          vkl_uint64 leafDataIndex,                                    \
          const vec3ui & /*offset*/,                                   \
          float time)                                                  \
  {                                                                    \
    return interpolateTemporallyUnstructured_##voxelType(              \
        grid->leafData + leafDataIndex,                                \
        grid->leafUnstructuredIndices + leafIndex,                     \
        grid->leafUnstructuredTimes + leafIndex,                       \
        0u,                                                            \
        time);                                                         \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_tile_unstructured(half)
__vkl_template_VdbSampler_sample_tile_unstructured(float)

#undef __vkl_template_VdbSampler_sample_tile_unstructured

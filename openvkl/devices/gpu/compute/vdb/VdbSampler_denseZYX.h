// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "temporal_data_interpolation.h"

// ---------------------------------------------------------------------------
// Value range.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_computeValueRange_denseZYX_constant(     \
    voxelType)                                                             \
  inline box1f VdbSampler_computeValueRange_denseZYX_constant_##voxelType( \
      const VdbGrid *grid,                                                 \
      uint64 /*leafIndex*/,                                                \
      uint64 leafDataIndex,                                                \
      const vec2ui &xRange,                                                \
      const vec2ui &yRange,                                                \
      const vec2ui &zRange)                                                \
  {                                                                        \
    box1f valueRange = make_box1f(pos_inf, neg_inf);                       \
    for (unsigned int x = xRange.x; x < xRange.y; ++x) {                   \
      for (unsigned int y = yRange.x; y < yRange.y; ++y) {                 \
        for (unsigned int z = zRange.x; z < zRange.y; ++z) {               \
          const uint64 voxelIdx =                                          \
              __vkl_vdb_domain_offset_to_linear_uniform_leaf(x, y, z);     \
          assert(voxelIdx < ((uint64)1) << 32);                            \
          const uint32 v32 = ((uint32)voxelIdx);                           \
          extend(valueRange,                                               \
                 get_##voxelType(grid->leafData[leafDataIndex], v32));     \
        }                                                                  \
      }                                                                    \
    }                                                                      \
    return valueRange;                                                     \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_denseZYX_constant(half)
__vkl_template_VdbSampler_computeValueRange_denseZYX_constant(float)

#undef __vkl_template_VdbSampler_computeValueRange_denseZYX_constant

#define __vkl_template_VdbSampler_computeValueRange_denseZYX_structured(     \
    voxelType)                                                               \
  inline box1f VdbSampler_computeValueRange_denseZYX_structured_##voxelType( \
      const VdbGrid *grid,                                                   \
      uint64 leafIndex,                                                      \
      uint64 leafDataIndex,                                                  \
      const vec2ui &xRange,                                                  \
      const vec2ui &yRange,                                                  \
      const vec2ui &zRange)                                                  \
  {                                                                          \
    const uint32 numTimesteps = grid->leafStructuredTimesteps[leafIndex];    \
    box1f valueRange          = make_box1f(pos_inf, neg_inf);                \
                                                                             \
    for (unsigned int x = xRange.x; x < xRange.y; ++x) {                     \
      for (unsigned int y = yRange.x; y < yRange.y; ++y) {                   \
        for (unsigned int z = zRange.x; z < zRange.y; ++z) {                 \
          const uint64 voxelIdx =                                            \
              numTimesteps *                                                 \
              __vkl_vdb_domain_offset_to_linear_uniform_leaf(x, y, z);       \
          for (unsigned int t = 0; t < numTimesteps; ++t) {                  \
            assert((voxelIdx + t) < ((uint64)1) << 32);                      \
            const uint32 v32 = ((uint32)(voxelIdx + t));                     \
            extend(valueRange,                                               \
                   get_##voxelType(grid->leafData[leafDataIndex], v32));     \
          }                                                                  \
        }                                                                    \
      }                                                                      \
    }                                                                        \
    return valueRange;                                                       \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_denseZYX_structured(half)
__vkl_template_VdbSampler_computeValueRange_denseZYX_structured(float)

#undef __vkl_template_VdbSampler_computeValueRange_denseZYX_structured

#define __vkl_template_VdbSampler_computeValueRange_denseZYX_unstructured(     \
    voxelType)                                                                 \
  inline box1f VdbSampler_computeValueRange_denseZYX_unstructured_##voxelType( \
      const VdbGrid *grid,                                                     \
      uint64 leafIndex,                                                        \
      uint64 leafDataIndex,                                                    \
      const vec2ui &xRange,                                                    \
      const vec2ui &yRange,                                                    \
      const vec2ui &zRange)                                                    \
  {                                                                            \
    box1f valueRange = make_box1f(pos_inf, neg_inf);                           \
                                                                               \
    for (unsigned int x = xRange.x; x < xRange.y; ++x) {                       \
      for (unsigned int y = yRange.x; y < yRange.y; ++y) {                     \
        for (unsigned int z = zRange.x; z < zRange.y; ++z) {                   \
          const uint64 voxelIdx =                                              \
              __vkl_vdb_domain_offset_to_linear_uniform_leaf(x, y, z);         \
          assert(voxelIdx < ((uint64)1) << 32);                                \
          const uint32 v32 = ((uint32)voxelIdx);                               \
          valueRange =                                                         \
              box_extend(valueRange,                                           \
                         computeValueRangeTemporallyUnstructured_##voxelType(  \
                             grid->leafData + leafDataIndex,                   \
                             grid->leafUnstructuredIndices + leafIndex,        \
                             v32));                                            \
        }                                                                      \
      }                                                                        \
    }                                                                          \
                                                                               \
    return valueRange;                                                         \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_denseZYX_unstructured(half)
__vkl_template_VdbSampler_computeValueRange_denseZYX_unstructured(float)

#undef __vkl_template_VdbSampler_computeValueRange_denseZYX_unstructured

// ---------------------------------------------------------------------------
// Constant leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_denseZYX_constant(voxelType)       \
  inline float                                                              \
      VdbSampler_sample_uniform_uniform_denseZYX_constant_##voxelType(      \
          const VdbGrid *grid,                                              \
          vkl_uint64 /*leafIndex*/,                                         \
          vkl_uint64 leafDataIndex,                                         \
          const vec3ui &offset,                                             \
          float /*time*/)                                                   \
  {                                                                         \
    const uint64 voxelIdx = __vkl_vdb_domain_offset_to_linear_uniform_leaf( \
        offset.x, offset.y, offset.z);                                      \
    assert(voxelIdx < ((uint64)1) << 32);                                   \
    const uint32 v32 = ((uint32)voxelIdx);                                  \
                                                                            \
    return get_##voxelType(grid->leafData[leafDataIndex], v32);             \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_denseZYX_constant(half)
__vkl_template_VdbSampler_sample_denseZYX_constant(float)

#undef __vkl_template_VdbSampler_sample_denseZYX_constant

// ---------------------------------------------------------------------------
// Structured leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_denseZYX_structured(voxelType)  \
  inline float                                                           \
      VdbSampler_sample_uniform_uniform_denseZYX_structured_##voxelType( \
          const VdbGrid *grid,                                           \
          vkl_uint64 leafIndex,                                          \
          vkl_uint64 leafDataIndex,                                      \
          const vec3ui &offset,                                          \
          float time)                                                    \
  {                                                                      \
    const int32 numTimesteps = grid->leafStructuredTimesteps[leafIndex]; \
    const uint64 voxelIdx =                                              \
        numTimesteps * __vkl_vdb_domain_offset_to_linear_uniform_leaf(   \
                           offset.x, offset.y, offset.z);                \
    assert(voxelIdx < ((uint64)1) << 32);                                \
    const uint32 v32 = ((uint32)voxelIdx);                               \
                                                                         \
    return interpolateTemporallyStructured_##voxelType(                  \
        grid->leafData + leafDataIndex, numTimesteps, v32, time);        \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_denseZYX_structured(half)
__vkl_template_VdbSampler_sample_denseZYX_structured(float)

#undef __vkl_template_VdbSampler_sample_denseZYX_structured

// ---------------------------------------------------------------------------
// Unstructured leaf sampling.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_sample_denseZYX_unstructured(voxelType)     \
  inline float                                                                \
      VdbSampler_sample_uniform_uniform_denseZYX_unstructured_##voxelType(    \
          const VdbGrid *grid,                                                \
          vkl_uint64 leafIndex,                                               \
          vkl_uint64 leafDataIndex,                                           \
          const vec3ui &offset,                                               \
          float time)                                                         \
  {                                                                           \
    const uint64 voxelIdx64 = __vkl_vdb_domain_offset_to_linear_uniform_leaf( \
        offset.x, offset.y, offset.z);                                        \
    assert(voxelIdx64 < ((uint64)1) << 32);                                   \
    const uint32 voxelIdx = ((uint32)voxelIdx64);                             \
                                                                              \
    return interpolateTemporallyUnstructured_##voxelType(                     \
        grid->leafData + leafDataIndex,                                       \
        grid->leafUnstructuredIndices + leafIndex,                            \
        grid->leafUnstructuredTimes + leafIndex,                              \
        voxelIdx,                                                             \
        time);                                                                \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_denseZYX_unstructured(half)
__vkl_template_VdbSampler_sample_denseZYX_unstructured(float)

#undef __vkl_template_VdbSampler_sample_denseZYX_unstructured

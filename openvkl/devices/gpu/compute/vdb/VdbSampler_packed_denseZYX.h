// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ---------------------------------------------------------------------------
// Value range.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_computeValueRange_packed_denseZYX(voxelType) \
  inline box1f VdbSampler_computeValueRange_packed_denseZYX_##voxelType##_32(  \
      const VdbGrid *grid,                                                     \
      uint64 leafIndex,                                                        \
      uint32 attributeIndex,                                                   \
      const vec2ui &xRange,                                                    \
      const vec2ui &yRange,                                                    \
      const vec2ui &zRange)                                                    \
  {                                                                            \
    box1f valueRange = make_box1f(pos_inf, neg_inf);                           \
    for (unsigned int x = xRange.x; x < xRange.y; ++x) {                       \
      for (unsigned int y = yRange.x; y < yRange.y; ++y) {                     \
        for (unsigned int z = zRange.x; z < zRange.y; ++z) {                   \
          const uint64 voxelIdx =                                              \
              leafIndex * VKL_VDB_NUM_VOXELS_LEAF +                            \
              __vkl_vdb_domain_offset_to_linear_uniform_leaf(x, y, z);         \
          assert(voxelIdx < ((uint64)1) << 32);                                \
          const uint32 v32 = ((uint32)voxelIdx);                               \
          extend(valueRange,                                                   \
                 get_##voxelType##_compact(                                    \
                     grid->nodesPackedDense[attributeIndex], v32));            \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return valueRange;                                                         \
  }                                                                            \
                                                                               \
  inline box1f VdbSampler_computeValueRange_packed_denseZYX_##voxelType##_64(  \
      const VdbGrid *grid,                                                     \
      uint64 leafIndex,                                                        \
      uint32 attributeIndex,                                                   \
      const vec2ui &xRange,                                                    \
      const vec2ui &yRange,                                                    \
      const vec2ui &zRange)                                                    \
  {                                                                            \
    box1f valueRange = make_box1f(pos_inf, neg_inf);                           \
    for (unsigned int x = xRange.x; x < xRange.y; ++x) {                       \
      for (unsigned int y = yRange.x; y < yRange.y; ++y) {                     \
        for (unsigned int z = zRange.x; z < zRange.y; ++z) {                   \
          const uint64 voxelIdx =                                              \
              leafIndex * VKL_VDB_NUM_VOXELS_LEAF +                            \
              __vkl_vdb_domain_offset_to_linear_uniform_leaf(x, y, z);         \
          extend(valueRange,                                                   \
                 get_##voxelType(grid->nodesPackedDense[attributeIndex],       \
                                 voxelIdx));                                   \
        }                                                                      \
      }                                                                        \
    }                                                                          \
    return valueRange;                                                         \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_packed_denseZYX(half);
__vkl_template_VdbSampler_computeValueRange_packed_denseZYX(float);

#undef __vkl_template_VdbSampler_computeValueRange_packed_denseZYX

// ---------------------------------------------------------------------------
// Constant leaf sampling.
// ---------------------------------------------------------------------------

// Currently this approach is only used for temporally-constant, denseZYX leaf
// nodes.

#define __vkl_template_VdbSampler_sample_packed_denseZYX(voxelType)           \
  /* for 32-bit addressing mode */                                            \
  inline float                                                                \
      VdbSampler_sample_uniform_uniform_packed_denseZYX_##voxelType##_32(     \
          const VdbGrid *grid,                                                \
          vkl_uint64 leafIndex,                                               \
          vkl_uint32 attributeIndex,                                          \
          const vec3ui &offset,                                               \
          float /*time*/)                                                     \
  {                                                                           \
    const uint64 voxelIdx = leafIndex * VKL_VDB_NUM_VOXELS_LEAF +             \
                            __vkl_vdb_domain_offset_to_linear_uniform_leaf(   \
                                offset.x, offset.y, offset.z);                \
    assert(voxelIdx < ((uint64)1) << 32);                                     \
    const uint32 v32 = ((uint32)voxelIdx);                                    \
                                                                              \
    return get_##voxelType##_compact(grid->nodesPackedDense[attributeIndex],  \
                                     v32);                                    \
  }                                                                           \
                                                                              \
  /* for 64-bit addressing mode */                                            \
  inline float                                                                \
      VdbSampler_sample_uniform_uniform_packed_denseZYX_##voxelType##_64(     \
          const VdbGrid *grid,                                                \
          vkl_uint64 leafIndex,                                               \
          vkl_uint32 attributeIndex,                                          \
          const vec3ui &offset,                                               \
          float /*time*/)                                                     \
  {                                                                           \
    const uint64 voxelIdx = leafIndex * VKL_VDB_NUM_VOXELS_LEAF +             \
                            __vkl_vdb_domain_offset_to_linear_uniform_leaf(   \
                                offset.x, offset.y, offset.z);                \
    return get_##voxelType(grid->nodesPackedDense[attributeIndex], voxelIdx); \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_packed_denseZYX(half);
__vkl_template_VdbSampler_sample_packed_denseZYX(float);

#undef __vkl_template_VdbSampler_sample_packed_denseZYX

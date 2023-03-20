// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ---------------------------------------------------------------------------
// Value range.
// ---------------------------------------------------------------------------

#define __vkl_template_VdbSampler_computeValueRange_packed_tile(voxelType) \
  inline box1f VdbSampler_computeValueRange_packed_tile_##voxelType##_32(  \
      const VdbGrid *grid,                                                 \
      uint64 leafIndex,                                                    \
      uint32 attributeIndex,                                               \
      const vec2ui &xRange,                                                \
      const vec2ui &yRange,                                                \
      const vec2ui &zRange)                                                \
  {                                                                        \
    const float v = get_##voxelType##_compact(                             \
        grid->nodesPackedTile[attributeIndex], leafIndex);                 \
    return make_box1f(v, v);                                               \
  }                                                                        \
                                                                           \
  inline box1f VdbSampler_computeValueRange_packed_tile_##voxelType##_64(  \
      const VdbGrid *grid,                                                 \
      uint64 leafIndex,                                                    \
      uint32 attributeIndex,                                               \
      const vec2ui &xRange,                                                \
      const vec2ui &yRange,                                                \
      const vec2ui &zRange)                                                \
  {                                                                        \
    const float v =                                                        \
        get_##voxelType(grid->nodesPackedTile[attributeIndex], leafIndex); \
    return make_box1f(v, v);                                               \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_computeValueRange_packed_tile(half);
__vkl_template_VdbSampler_computeValueRange_packed_tile(float);

#undef __vkl_template_VdbSampler_computeValueRange_packed_tile

// ---------------------------------------------------------------------------
// Constant leaf sampling.
// ---------------------------------------------------------------------------

// Currently this approach is only used for temporally-constant, tile leaf
// nodes.

#define __vkl_template_VdbSampler_sample_packed_tile(voxelType)                \
  /* for 32-bit addressing mode */                                             \
  inline float VdbSampler_sample_uniform_uniform_packed_tile_##voxelType##_32( \
      const VdbGrid *grid,                                                     \
      vkl_uint64 leafIndex,                                                    \
      vkl_uint32 attributeIndex,                                               \
      const vec3ui &offset,                                                    \
      float /*time*/)                                                          \
  {                                                                            \
    return get_##voxelType##_compact(grid->nodesPackedTile[attributeIndex],    \
                                     leafIndex);                               \
  }                                                                            \
                                                                               \
  /* for 64-bit addressing mode */                                             \
  inline float VdbSampler_sample_uniform_uniform_packed_tile_##voxelType##_64( \
      const VdbGrid *grid,                                                     \
      vkl_uint64 leafIndex,                                                    \
      vkl_uint32 attributeIndex,                                               \
      const vec3ui &offset,                                                    \
      float /*time*/)                                                          \
  {                                                                            \
    return get_##voxelType(grid->nodesPackedTile[attributeIndex], leafIndex);  \
  }

// Not currently supported on GPU:
// __vkl_template_VdbSampler_sample_packed_tile(half);
__vkl_template_VdbSampler_sample_packed_tile(float);

#undef __vkl_template_VdbSampler_sample_packed_tile

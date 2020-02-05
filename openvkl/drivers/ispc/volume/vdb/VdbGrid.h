// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "openvkl/ispc_cpp_interop.h"
#include "openvkl/vdb.h"

#if defined(ISPC)

#include "math/box.ih"
#include "math/math.ih"
#include "math/vec.ih"

#elif defined(__cplusplus)

#include "../common/math.h"

namespace openvkl {
  namespace ispc_driver {

#endif  // defined(__cplusplus)

struct VdbLevel
{
  vkl_uint64 numNodes;

  // Voxels for node 0, node 1, node 2, ...
  vkl_uint64 *voxels;

  // For each voxel, the original leaf index.
  // Note: These are only valid for leaf and tile voxels.
  vkl_uint64 *leafIndex;

  // For each voxel, the range of values contained within.
  range1f *valueRange;
};

/*
 * A grid is a collection of levels.
 */
struct VdbGrid
{
  vkl_uint32 type;  // All voxels have this type.
  VKLFilter filter;
  vkl_uint32 maxSamplingDepth;
  vkl_uint32 maxIteratorDepth;
  float objectToIndex[12];    // Row-major transformation matrix, 3x4,
                              // rotation-shear-scale | translation
  float indexToObject[12];    // Row-major transformation matrix, 3x4,
                              // rotation-shear-scale | translation
  vkl_uint64 totalNumLeaves;  // The total number of leaf nodes in this tree.
  vkl_uint64
      numLeaves[VKL_VDB_NUM_LEVELS];  // The number of leaf nodes per level.
  vkl_uint64 maxVoxelOffset;  // Used to select 64bit or 32bit traversal. TODO:
                              // Use this in VDBSampler.ispc
  vec3i rootOrigin;           // In index space.
  vkl_uint32
      *usageBuffer;  // Nonzero if the given input leaf has been accessed.
  VdbLevel levels[VKL_VDB_NUM_LEVELS - 1];
};

/*
 * Transform points and vectors with the given affine matrix (in row major
 * order).
 */
#define __vkl_vdb_xfm_functions(univary)                      \
  inline univary vec3f xfmVector(                             \
      const VKL_INTEROP_UNIFORM float *VKL_INTEROP_UNIFORM M, \
      const univary vec3f &p)                                 \
  {                                                           \
    univary vec3f r;                                          \
    r.x = M[0] * p.x + M[1] * p.y + M[2] * p.z;               \
    r.y = M[3] * p.x + M[4] * p.y + M[5] * p.z;               \
    r.z = M[6] * p.x + M[7] * p.y + M[8] * p.z;               \
    return r;                                                 \
  }                                                           \
  inline univary vec3f xfmPoint(                              \
      const VKL_INTEROP_UNIFORM float *VKL_INTEROP_UNIFORM M, \
      const univary vec3f &p)                                 \
  {                                                           \
    univary vec3f r = xfmVector(M, p);                        \
    r.x             = r.x + M[9];                             \
    r.y             = r.y + M[10];                            \
    r.z             = r.z + M[11];                            \
    return r;                                                 \
  }

__vkl_interop_univary(__vkl_vdb_xfm_functions)
#undef __vkl_vdb_xfm_functions

    // ==========================================================================
    // // Voxel encoding
    //
    // empty    : 00 ... 00000
    // tile     : VV ... 00001 (32 bit tile value, 30 bit empty, 2 bit type)
    // child    : II ... III10 (62 bit index,   2 bit node type)
    // leaf     : PP ... PTT11 (60 bit pointer, 2 bit time format, 2 bit node
    // type)
    //
    // - Child node indices are extracted by masking the lower two bits. This
    // means that indices must be multiples of 4 (which is always true given
    // that voxels are 8 bytes apart).
    //
    // - In 32 bit mode, only the first 32 bits are read, the second 32 bits are
    // ignored.  32 bit mode is only used if the high bits are 0 for all voxels
    // in the tree.
    //
    // - Tile values are stored exclusively in the high bits.
    //
    // - Timesteps can be 00 (temporally unstructured), 01 (const), or 10
    // (temporally structured).
    //
    // - The lower 4 bits of leaf pointers are used for timestep and type
    // information, which means that leaf data pointers must be aligned to 16
    // byte boundaries. VKLVdb will reject other pointers.
    // ==========================================================================
    // //

    inline VKL_INTEROP_UNIFORM vkl_uint64 vklVdbVoxelMakeEmpty()
{
  return 0;
}

#define __vkl_vdb_define_voxeltype_functions(univary)                          \
                                                                               \
  inline univary bool vklVdbVoxelIsEmpty(univary vkl_uint32 voxel)             \
  {                                                                            \
    return ((voxel & 0x3u) == 0x0u);                                           \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeTile(univary float value)           \
  {                                                                            \
    const univary vkl_uint32 bits = intbits(value);                            \
    return ((((univary vkl_uint64)bits) << 32) + 0x1u);                        \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsTile(univary vkl_uint32 voxel)              \
  {                                                                            \
    return ((voxel & 0x3u) == 0x1u);                                           \
  }                                                                            \
                                                                               \
  inline univary float vklVdbVoxelTileGet(univary vkl_uint64 voxel)            \
  {                                                                            \
    const univary vkl_uint32 value = ((univary vkl_uint32)(voxel >> 32));      \
    return floatbits(value);                                                   \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeChildPtr(                           \
      univary vkl_uint64 childIdx)                                             \
  {                                                                            \
    assert((childIdx & (((vkl_uint64)0x3u) << 62)) == 0);                      \
    return (childIdx << 2) | 0x2u;                                             \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsChildPtr(univary vkl_uint32 voxel)          \
  {                                                                            \
    return ((voxel & 0x3u) == 0x2u);                                           \
  }                                                                            \
                                                                               \
  /* 32 bit addressing mode */                                                 \
  inline univary vkl_uint32 vklVdbVoxelChildGetIndex(univary vkl_uint32 voxel) \
  {                                                                            \
    return (voxel >> 2);                                                       \
  }                                                                            \
                                                                               \
  /* 64 bit addressing mode */                                                 \
  inline univary vkl_uint64 vklVdbVoxelChildGetIndex(univary vkl_uint64 voxel) \
  {                                                                            \
    return (voxel >> 2);                                                       \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeLeafPtr(                            \
      const void *univary leafPtr, univary VKLVdbLeafFormat format)            \
  {                                                                            \
    const univary vkl_uint64 intptr = ((univary vkl_uint64)leafPtr);           \
    assert((intptr & 0xFu) == 0); /* Require 16 Byte alignment! */             \
    const univary vkl_uint64 voxel =                                           \
        (intptr & ~((univary vkl_uint64)0xFu)) +                               \
        ((((univary vkl_uint64)format) & 0x3u) << 2) + 0x3u;                   \
    assert((const void *univary)(voxel & ~((univary vkl_uint64)0xFu)) ==       \
           leafPtr);                                                           \
    return voxel;                                                              \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsLeafPtr(univary vkl_uint32 voxel)           \
  {                                                                            \
    return ((voxel & 0x3u) == 0x3u);                                           \
  }                                                                            \
                                                                               \
  inline univary VKLVdbLeafFormat vklVdbVoxelLeafGetFormat(                    \
      univary vkl_uint32 voxel)                                                \
  {                                                                            \
    return ((VKLVdbLeafFormat)((voxel >> 2) & 0x3u));                          \
  }                                                                            \
  /* Leaf pointers are always 64 bit */                                        \
  inline const void *univary vklVdbVoxelLeafGetPtr(univary vkl_uint64 voxel)   \
  {                                                                            \
    return ((const void *univary)(voxel & ~((univary vkl_uint64)0xFu)));       \
  }

__vkl_interop_univary(__vkl_vdb_define_voxeltype_functions)

#undef __vkl_vdb_define_voxeltype_functions

#if defined(__cplusplus)

}  // namespace ispc_driver
}  // namespace openvkl

#endif  // defined(__cplusplus)

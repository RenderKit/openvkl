// Copyright 2019-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/VKLFormat.h"
#include "openvkl/VKLTemporalFormat.h"
#include "openvkl/ispc_cpp_interop.h"
#include "openvkl/vdb.h"

#if defined(ISPC)

#include "../common/Data.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/math.ih"
#include "rkcommon/math/vec.ih"

#elif defined(__cplusplus)

#include "../common/Data.h"
#include "../common/math.h"

namespace openvkl {
  namespace cpu_device {

    using ispc::Data1D;

#endif  // defined(__cplusplus)

struct VdbLevel
{
  vkl_uint64 numNodes;

  // For each *node*, the origin in root-relative index space.
  vec3ui *origin;

  // Voxels for node 0, node 1, node 2, ...
  vkl_uint64 *voxels;

  // For each voxel, the range of values contained within, by attribute:
  // (range0, range1, ..., rangeNumAttributes)_0, [...]
  range1f *valueRange;
};

/*
 * A grid is a collection of levels.
 */
struct VdbGrid
{
  // Global grid parameters.
  float objectToIndex[12];  // Row-major transformation matrix, 3x4,
                            // rotation-shear-scale | translation
  float indexToObject[12];  // Row-major transformation matrix, 3x4,
                            // rotation-shear-scale | translation
  vec3i rootOrigin;         // In index scale space.
  vec3ui activeSize;        // Size of the root node, in voxels,
  box3f domainBoundingBox;  // Domain space bounding box (different
                            // for constant cell data vs vertex centered data
                            // interpretation)
  bool constantCellData;    // Data is either considered constant per cell, or
                            // vertex-centered.

  vkl_uint64 numLeaves;
  vkl_uint32 numAttributes;
  bool allLeavesCompact;   // Do we only have compact (non strided) leaf data?
  bool allLeavesConstant;  // Are all leaf nodes temporally constant?

  vkl_uint32 *attributeTypes;  // Data type for each attribute.

  // Per-node data for sparse / non-dense volumes only.
  const vkl_int32 *leafStructuredTimesteps;
  Data1D *leafUnstructuredIndices;
  Data1D *leafUnstructuredTimes;

  // Per-attribute data for sparse / non-dense volumes only: size [numLeaves *
  // numAttributes]
  Data1D *leafData;

  // Optional: per-attribute node-packed data for sparse / non-dense volumes
  // only: size [numAttributes]
  bool packedAddressing32;
  Data1D *nodesPackedDense;
  Data1D *nodesPackedTile;

  // Parameters for dense volumes only.
  bool dense;
  vec3i denseDimensions;
  Data1D *denseData;  // Per-attribute data: size [numAttributes]
  VKLTemporalFormat denseTemporalFormat;
  int denseTemporallyStructuredNumTimesteps;
  Data1D denseTemporallyUnstructuredIndices;
  Data1D denseTemporallyUnstructuredTimes;

  // Level data.
  VdbLevel levels[VKL_VDB_NUM_LEVELS - 1];
};

/*
 * Obtain a pointer to the given leaf node and attribute.
 */
#define __vkl_vdb_get_leaf_data_index(univary)                                 \
  inline const univary vkl_uint64 vklVdbGetLeafDataIndex(                      \
      const VdbGrid *VKL_INTEROP_UNIFORM grid,                                 \
      univary vkl_uint64 leafIndex,                                            \
      VKL_INTEROP_UNIFORM vkl_uint32 attributeIndex)                           \
  {                                                                            \
    assert(grid);                                                              \
    assert(leafIndex < grid->numLeaves);                                       \
    assert(attributeIndex < grid->numAttributes);                              \
    return ((VKL_INTEROP_UNIFORM vkl_uint64)grid->numAttributes) * leafIndex + \
           ((VKL_INTEROP_UNIFORM vkl_uint64)attributeIndex);                   \
  }

__vkl_interop_univary(__vkl_vdb_get_leaf_data_index)
#undef __vkl_vdb_get_leaf_data_index

/*
 * Transform points and vectors with the given affine matrix (in row major
 * order).
 *
 * Note that xfmNormal takes the inverse matrix, as the normal transform matrix
 * for a given matrix M is (M^{-1})^T.
 */
#define __vkl_vdb_xfm_functions(univary)                         \
  inline univary vec3f xfmVector(                                \
      const VKL_INTEROP_UNIFORM float *VKL_INTEROP_UNIFORM M,    \
      const univary vec3f &p)                                    \
  {                                                              \
    univary vec3f r;                                             \
    r.x = M[0] * p.x + M[1] * p.y + M[2] * p.z;                  \
    r.y = M[3] * p.x + M[4] * p.y + M[5] * p.z;                  \
    r.z = M[6] * p.x + M[7] * p.y + M[8] * p.z;                  \
    return r;                                                    \
  }                                                              \
  inline univary vec3f xfmNormal(                                \
      const VKL_INTEROP_UNIFORM float *VKL_INTEROP_UNIFORM MInv, \
      const univary vec3f &p)                                    \
  {                                                              \
    univary vec3f r;                                             \
    r.x = MInv[0] * p.x + MInv[3] * p.y + MInv[6] * p.z;         \
    r.y = MInv[1] * p.x + MInv[4] * p.y + MInv[7] * p.z;         \
    r.z = MInv[2] * p.x + MInv[5] * p.y + MInv[8] * p.z;         \
    return r;                                                    \
  }                                                              \
  inline univary vec3f xfmPoint(                                 \
      const VKL_INTEROP_UNIFORM float *VKL_INTEROP_UNIFORM M,    \
      const univary vec3f &p)                                    \
  {                                                              \
    univary vec3f r = xfmVector(M, p);                           \
    r.x             = r.x + M[9];                                \
    r.y             = r.y + M[10];                               \
    r.z             = r.z + M[11];                               \
    return r;                                                    \
  }

__vkl_interop_univary(__vkl_vdb_xfm_functions)
#undef __vkl_vdb_xfm_functions

/*******************************************************************************
 *
 * Voxel encoding
 *
 * empty    : 00 ... 00000
 * error    : V .. L .. 01       (32 bit voxel offset, 16 bit empty, 8 bit voxel
 *                                level, 6 bit empty, 2 bit type)
 * child    : II ... III 10      (62 bit index, 2 bit type)
 * leaf     : II .. II TF DF 11  (58 bit leaf index, 2 bit temporal format,
 *                                2 bit data format, 2 bit type)
 *
 ******************************************************************************/

/*
 * This is the maximum number of leaf data pointers we can store, based on the
 * size of the index.
 * Note that numLeaves * numAttributes must be less than this value!
 */
#define VKL_VDB_MAX_NUM_LEAF_DATA \
  ((((VKL_INTEROP_UNIFORM vkl_uint64)1) << 58) - 1)

inline VKL_INTEROP_UNIFORM vkl_uint64 vklVdbVoxelMakeEmpty()
{
  return 0;
}

#define __vkl_vdb_define_voxeltype_functions(univary)                          \
                                                                               \
  inline univary bool vklVdbVoxelIsEmpty(univary vkl_uint64 voxel)             \
  {                                                                            \
    return ((voxel & 0x3u) == 0x0u);                                           \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeError(univary uint8 level,          \
                                                 univary uint32 voxelOffset)   \
  {                                                                            \
    const univary vkl_uint64 voxel =                                           \
        (((univary vkl_uint64)voxelOffset) << 32) +                            \
        (((univary vkl_uint64)level) << 8) + 0x1u;                             \
    assert((univary uint8)(voxel >> 8) == level);                              \
    assert((univary uint32)(voxel >> 32) == voxelOffset);                      \
    return voxel;                                                              \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsError(univary vkl_uint64 voxel)             \
  {                                                                            \
    return ((voxel & 0x3u) == 0x1u);                                           \
  }                                                                            \
                                                                               \
  inline void vklVdbVoxelErrorGet(univary vkl_uint64 voxel,                    \
                                  univary uint8 &level,                        \
                                  univary uint32 &voxelOffset)                 \
  {                                                                            \
    level       = voxel >> 8;                                                  \
    voxelOffset = voxel >> 32;                                                 \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeChildPtr(                           \
      univary vkl_uint64 childIdx)                                             \
  {                                                                            \
    assert((childIdx & (((vkl_uint64)0x3u) << 62)) == 0);                      \
    return (childIdx << 2) | 0x2u;                                             \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsChildPtr(univary vkl_uint64 voxel)          \
  {                                                                            \
    return ((voxel & 0x3u) == 0x2u);                                           \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelChildGetIndex(univary vkl_uint64 voxel) \
  {                                                                            \
    return (voxel >> 2);                                                       \
  }                                                                            \
                                                                               \
  inline univary bool vklVdbVoxelIsLeafPtr(univary vkl_uint64 voxel)           \
  {                                                                            \
    return ((voxel & 0x3u) == 0x3u);                                           \
  }                                                                            \
                                                                               \
  inline univary VKLFormat vklVdbVoxelLeafGetFormat(univary vkl_uint64 voxel)  \
  {                                                                            \
    return ((univary VKLFormat)((voxel >> 2) & 0x3u));                         \
  }                                                                            \
                                                                               \
  inline univary VKLTemporalFormat vklVdbVoxelLeafGetTemporalFormat(           \
      univary vkl_uint64 voxel)                                                \
  {                                                                            \
    return ((univary VKLTemporalFormat)((voxel >> 4) & 0x3u));                 \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelLeafGetIndex(univary vkl_uint64 voxel)  \
  {                                                                            \
    return ((univary vkl_uint64)(voxel >> 6));                                 \
  }                                                                            \
                                                                               \
  inline univary vkl_uint64 vklVdbVoxelMakeLeafPtr(                            \
      univary vkl_uint64 leafIndex,                                            \
      univary VKLFormat format,                                                \
      univary VKLTemporalFormat temporalFormat)                                \
  {                                                                            \
    assert(format <= 4);                                                       \
    assert(temporalFormat <= 4);                                               \
    assert(leafIndex < (((univary vkl_uint64)1) << 58));                       \
    const univary vkl_uint64 voxel =                                           \
        (((univary vkl_uint64)leafIndex) << 6) +                               \
        ((((univary vkl_uint64)temporalFormat) & 0x3u) << 4) +                 \
        ((((univary vkl_uint64)format) & 0x3u) << 2) + 0x3u;                   \
    assert(vklVdbVoxelIsLeafPtr(voxel));                                       \
    assert(vklVdbVoxelLeafGetFormat(voxel) == format);                         \
    assert(vklVdbVoxelLeafGetTemporalFormat(voxel) == temporalFormat);         \
    assert(vklVdbVoxelLeafGetIndex(voxel) == leafIndex);                       \
    return voxel;                                                              \
  }

__vkl_interop_univary(__vkl_vdb_define_voxeltype_functions)

#undef __vkl_vdb_define_voxeltype_functions

#if defined(__cplusplus)

}  // namespace cpu_device
}  // namespace openvkl

#endif  // defined(__cplusplus)

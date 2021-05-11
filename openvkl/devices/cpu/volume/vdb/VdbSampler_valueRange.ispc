// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbGrid.h"
#include "VdbSampler.ih"
#include "VdbSampler_denseZYX.ih"
#include "VdbSampler_leafHandler.ih"
#include "VdbSampler_tile.ih"
#include "VdbSampler_traverseAndSample.ih"
#include "common/export_util.h"

// ---------------------------------------------------------------------------
// Value range computation.
// ---------------------------------------------------------------------------

/*
 * Compute the value range for the given node and index range.
 */
inline uniform box1f
VdbSampler_computeNodeValueRange(const VdbGrid *uniform grid,
                                 uniform uint64 voxel,
                                 const uniform vec2ui &rangeX,
                                 const uniform vec2ui &rangeY,
                                 const uniform vec2ui &rangeZ,
                                 uniform uint32 attributeIndex)
{
  if (!vklVdbVoxelIsLeafPtr(voxel)) {
    const uniform float background = 0.f;
    return make_box1f(background, background);
  }

  const uniform vkl_uint32 voxelType = grid->attributeTypes[attributeIndex];
  const uniform VKLFormat leafFormat = vklVdbVoxelLeafGetFormat(voxel);
  const uniform VKLTemporalFormat leafTemporalFormat =
      vklVdbVoxelLeafGetTemporalFormat(voxel);
  const uniform uint64 leafIndex = vklVdbVoxelLeafGetIndex(voxel);
  const uniform uint64 leafDataIndex =
      vklVdbGetLeafDataIndex(grid, leafIndex, attributeIndex);

  __vkl_vdb_leaf_handler(return VdbSampler_computeValueRange,
                                voxelType,
                                leafFormat,
                                leafTemporalFormat,
                                grid,
                                leafIndex,
                                leafDataIndex,
                                rangeX,
                                rangeY,
                                rangeZ)
}

/*
 * Compute the value range on the given constant float leaf.
 */
export void EXPORT_UNIQUE(VdbSampler_computeValueRange,
                          const void *uniform _grid,
                          const vec3ui *uniform centerNodeOffset,
                          uniform uint32 level,
                          uniform uint32 attributeIndex,
                          uniform box1f *uniform range)
{
  const VdbGrid *uniform grid = (const VdbGrid *uniform)_grid;

  // We need to consider voxels outside this node in the value range because
  // of reconstruction filter support.
  //
  // In particular, we support filters that use up to a 4x4x4 neighborhood.
  //
  // In the following 1D illustration, we show voxels in the current leaf node
  // marked as a '#'. We also show voxels from neighboring nodes, and
  // mark those that could be used for interpolation with a '?'.
  //
  // | | | | | | | |?|#|#|#|#|#|#|#|#|?|?| | | | | | |
  //
  // We therefore iterate over a 3x3x3 neighborhood of nodes and compute the
  // value range over the following voxel range (in each dimension):
  //
  // "previous node" : [VKL_VDB_RES_LEAF - 1, VKL_VDB_RES_LEAF[
  // "central node"  : [0, VKL_VDB_RES_LEAF[
  // "next node"     : [0, 2[
  //
  // Note that the upper limit is exclusive. In 3D, each dimension follows
  // the same pattern.
  //
  // The following array encodes the iteration range for the previous,
  // central, and next leaf node:
  const uniform vec2ui voxelRange[] = {
      make_vec2ui(VKL_VDB_RES_LEAF - 1, VKL_VDB_RES_LEAF),
      make_vec2ui(0, VKL_VDB_RES_LEAF),
      make_vec2ui(0, 2)};

  range->lower = pos_inf;
  range->upper = neg_inf;

  uniform VdbSampler sampler;
  memset(&sampler, 0, sizeof(uniform VdbSampler));
  sampler.grid             = (const VdbGrid *uniform)_grid;
  sampler.maxSamplingDepth = VKL_VDB_NUM_LEVELS - 1;

  // Iterate over a 3x3x3 neighborhood of leaf nodes, and compute the value
  // range for each of those nodes.
  for (uniform int ix = 0; ix < 3; ++ix) {
    for (uniform int iy = 0; iy < 3; ++iy) {
      for (uniform int iz = 0; iz < 3; ++iz) {
        // This is the offset of the current node relative to the central
        // node.
        const uniform vec3i nodeOffset =
            make_vec3i((ix - 1) * VKL_VDB_RES_LEAF,
                       (iy - 1) * VKL_VDB_RES_LEAF,
                       (iz - 1) * VKL_VDB_RES_LEAF);

        // *centerNodeOffset is relative to the rootOrigin, but
        // VdbSampler_traverse expects an index space coordinate.
        const uniform vec3i nodeOrigin =
            make_vec3i(grid->rootOrigin.x + centerNodeOffset->x + nodeOffset.x,
                       grid->rootOrigin.y + centerNodeOffset->y + nodeOffset.y,
                       grid->rootOrigin.z + centerNodeOffset->z + nodeOffset.z);

        uniform uint64 voxel;
        uniform vec3ui domainOffset;
        VdbSampler_traverse(&sampler, nodeOrigin, voxel, domainOffset);

        const uniform range1f nodeValueRange =
            VdbSampler_computeNodeValueRange(grid,
                                             voxel,
                                             voxelRange[ix],
                                             voxelRange[iy],
                                             voxelRange[iz],
                                             attributeIndex);

        range->lower = min(range->lower, nodeValueRange.lower);
        range->upper = max(range->upper, nodeValueRange.upper);
      }
    }
  }
}


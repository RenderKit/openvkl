// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "../common/Data.ih"
#include "VdbLeafAccessObserver.ih"
#include "VdbSampleConstantLeaf.ih"
#include "VdbSampleTileLeaf.ih"
#include "VdbSampler.ih"
#include "VdbVolume.ih"
#include "common/export_util.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.ih"

// ---------------------------------------------------------------------------
// Helper functions used in sampling voxels
// ---------------------------------------------------------------------------

// This helper function handles all legal combinations of sampling uniform /
// varying voxels at uniform / varying offsets, and is used in the main
// entry-point sample functions below.
#define template_VdbSampler_sample_inner(univaryVoxel, univaryOffset)       \
  inline univaryOffset float VdbSampler_sample_inner(                       \
      const VdbSampler *uniform sampler,                                    \
      const univaryVoxel uint64 &voxel,                                     \
      const univaryOffset vec3ui &domainOffset,                             \
      const uniform uint32 attributeIndex)                                  \
  {                                                                         \
    univaryOffset float sample = 0.f;                                       \
                                                                            \
    if (vklVdbVoxelIsLeafPtr(voxel)) {                                      \
      const void *univaryVoxel leafPtr = vklVdbVoxelLeafGetPtr(voxel);      \
      assert(leafPtr);                                                      \
                                                                            \
      const uniform bool isCompact = sampler->grid->allLeavesCompact;       \
                                                                            \
      const univaryVoxel VKLFormat leafFormat =                             \
          vklVdbVoxelLeafGetFormat(voxel);                                  \
                                                                            \
      if (leafFormat == VKL_FORMAT_TILE) {                                  \
        sample = VdbSampler_sampleTileFloatLeaf(                            \
            ((const uniform Data1D *univaryVoxel)leafPtr), attributeIndex); \
      } else if (isCompact && leafFormat == VKL_FORMAT_CONSTANT_ZYX) {      \
        sample = VdbSampler_sampleConstantFloatLeafCompact(                 \
            ((const uniform Data1D *univaryVoxel)leafPtr),                  \
            domainOffset,                                                   \
            attributeIndex);                                                \
      } else if (!isCompact && leafFormat == VKL_FORMAT_CONSTANT_ZYX) {     \
        sample = VdbSampler_sampleConstantFloatLeafStrided(                 \
            ((const uniform Data1D *univaryVoxel)leafPtr),                  \
            domainOffset,                                                   \
            attributeIndex);                                                \
      } else {                                                              \
        assert(false);                                                      \
      }                                                                     \
    } else if (vklVdbVoxelIsError(voxel)) {                                 \
      univaryVoxel uint8 level;                                             \
      univaryVoxel uint32 voxelOffset;                                      \
      vklVdbVoxelErrorGet(voxel, level, voxelOffset);                       \
                                                                            \
      const univaryVoxel range1f valueRange =                               \
          sampler->grid->levels[level]                                      \
              .valueRange[voxelOffset * sampler->grid->numAttributes +      \
                          attributeIndex];                                  \
      sample = 0.5f * (valueRange.lower + valueRange.upper);                \
    }                                                                       \
                                                                            \
    return sample;                                                          \
  }

template_VdbSampler_sample_inner(varying, varying);
template_VdbSampler_sample_inner(uniform, varying);
template_VdbSampler_sample_inner(uniform, uniform);
#undef template_VdbSampler_sample_inner

// ---------------------------------------------------------------------------
// The main entrypoints for sampling a volume.
// These are called from the interpolation scheduling routines below.
// ---------------------------------------------------------------------------

/*
 * Sample a volume for given index coordinate (varying).
 */
inline varying float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const varying vec3i &ic,
                                       const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const vec3i rootOrg = sampler->grid->rootOrigin;
  if (ic.x < rootOrg.x || ic.y < rootOrg.y || ic.z < rootOrg.z)
    return 0.f;

  const vec3ui domainOffset = make_vec3ui(ic - rootOrg);
  if (domainOffset.x >= VKL_VDB_RES_0 || domainOffset.y >= VKL_VDB_RES_0 ||
      domainOffset.z >= VKL_VDB_RES_0) {
    return 0.f;
  }

  // We traverse the tree based on the leaf node origin, which we can easily
  // compute using bit masking operations:
  const uint32 originMask = ~(((uint32)(VKL_VDB_RES_LEAF - 1)));
  const vec3ui leafOrigin = make_vec3ui(domainOffset.x & originMask,
                                        domainOffset.y & originMask,
                                        domainOffset.z & originMask);

  float sample = 0.f;

  uniform vec3ui uniformLeafOrigin;
  if (reduce_equal(leafOrigin.x, &uniformLeafOrigin.x) &&
      reduce_equal(leafOrigin.y, &uniformLeafOrigin.y) &&
      reduce_equal(leafOrigin.z, &uniformLeafOrigin.z)) {
    // All queries are in the same leaf node. Do uniform traversal!
    uniform uint64 voxel = vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_uniform_0(
        sampler, 0ul, uniformLeafOrigin, voxel);

    sample =
        VdbSampler_sample_inner(sampler, voxel, domainOffset, attributeIndex);

  } else {
    // Varying traversal.
    uint64 voxel = vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_varying_0(sampler, 0ul, leafOrigin, voxel);

    sample =
        VdbSampler_sample_inner(sampler, voxel, domainOffset, attributeIndex);
  }

  return sample;
}

/*
 * Sample a volume for given index coordinate (uniform).
 */
inline uniform float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const uniform vec3i &ic,
                                       const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const uniform vec3i rootOrg = sampler->grid->rootOrigin;
  if (ic.x < rootOrg.x || ic.y < rootOrg.y || ic.z < rootOrg.z)
    return 0.f;

  const uniform vec3ui domainOffset = make_vec3ui(ic - rootOrg);
  if (domainOffset.x >= VKL_VDB_RES_0 || domainOffset.y >= VKL_VDB_RES_0 ||
      domainOffset.z >= VKL_VDB_RES_0) {
    return 0.f;
  }

  // We traverse the tree based on the leaf node origin, which we can easily
  // compute using bit masking operations:
  const uniform uint32 originMask = ~(((uint32)(VKL_VDB_RES_LEAF - 1)));
  const uniform vec3ui leafOrigin = make_vec3ui(domainOffset.x & originMask,
                                                domainOffset.y & originMask,
                                                domainOffset.z & originMask);

  uniform uint64 voxel = vklVdbVoxelMakeEmpty();
  VdbSampler_dispatchInner_uniform_uniform_0(sampler, 0ul, leafOrigin, voxel);

  return VdbSampler_sample_inner(sampler, voxel, domainOffset, attributeIndex);
}

/*
 * Traverse a volume for given index coordinate (varying).
 */
inline void VdbSampler_traverse(const VdbSampler *uniform sampler,
                                const varying vec3i &ic,
                                varying uint64 &voxel,
                                varying vec3ui &domainOffset)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const vec3i rootOrg = sampler->grid->rootOrigin;
  if (ic.x < rootOrg.x || ic.y < rootOrg.y || ic.z < rootOrg.z) {
    voxel = vklVdbVoxelMakeEmpty();
    return;
  }

  domainOffset = make_vec3ui(ic - rootOrg);
  if (domainOffset.x >= VKL_VDB_RES_0 || domainOffset.y >= VKL_VDB_RES_0 ||
      domainOffset.z >= VKL_VDB_RES_0) {
    voxel = vklVdbVoxelMakeEmpty();
    return;
  }

  // We traverse the tree based on the leaf node origin, which we can easily
  // compute using bit masking operations:
  const uint32 originMask = ~(((uint32)(VKL_VDB_RES_LEAF - 1)));
  const vec3ui leafOrigin = make_vec3ui(domainOffset.x & originMask,
                                        domainOffset.y & originMask,
                                        domainOffset.z & originMask);

  uniform vec3ui uniformLeafOrigin;
  if (reduce_equal(leafOrigin.x, &uniformLeafOrigin.x) &&
      reduce_equal(leafOrigin.y, &uniformLeafOrigin.y) &&
      reduce_equal(leafOrigin.z, &uniformLeafOrigin.z)) {
    // All queries are in the same leaf node. Do uniform traversal!
    uniform uint64 voxelU = vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_uniform_0(
        sampler, 0ul, uniformLeafOrigin, voxelU);

    voxel = voxelU;

  } else {
    // Varying traversal.
    voxel = vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_varying_0(sampler, 0ul, leafOrigin, voxel);
  }
}

/*
 * Traverse a volume for given index coordinate (uniform).
 */
inline void VdbSampler_traverse(const VdbSampler *uniform sampler,
                                const uniform vec3i &ic,
                                uniform uint64 &voxel,
                                uniform vec3ui &domainOffset)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const uniform vec3i rootOrg = sampler->grid->rootOrigin;
  if (ic.x < rootOrg.x || ic.y < rootOrg.y || ic.z < rootOrg.z) {
    voxel = vklVdbVoxelMakeEmpty();
    return;
  }

  domainOffset = make_vec3ui(ic - rootOrg);
  if (domainOffset.x >= VKL_VDB_RES_0 || domainOffset.y >= VKL_VDB_RES_0 ||
      domainOffset.z >= VKL_VDB_RES_0) {
    voxel = vklVdbVoxelMakeEmpty();
    return;
  }

  // We traverse the tree based on the leaf node origin, which we can easily
  // compute using bit masking operations:
  const uniform uint32 originMask = ~(((uint32)(VKL_VDB_RES_LEAF - 1)));
  const uniform vec3ui leafOrigin = make_vec3ui(domainOffset.x & originMask,
                                                domainOffset.y & originMask,
                                                domainOffset.z & originMask);

  voxel = vklVdbVoxelMakeEmpty();
  VdbSampler_dispatchInner_uniform_uniform_0(sampler, 0ul, leafOrigin, voxel);
}

/*
 * Sample a volume for given pre-traversed voxel and domain offset (varying).
 */
inline varying float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const varying uint64 &voxel,
                                       const varying vec3ui &domainOffset,
                                       const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  if (vklVdbVoxelIsEmpty(voxel)) {
    return 0.f;
  }

  float sample = 0.f;

  uniform uint64 uniformVoxel;
  if (reduce_equal(voxel, &uniformVoxel)) {
    // All queries are in the same leaf node. Do uniform sampling!
    sample = VdbSampler_sample_inner(
        sampler, uniformVoxel, domainOffset, attributeIndex);

  } else {
    // Varying sampling.
    sample =
        VdbSampler_sample_inner(sampler, voxel, domainOffset, attributeIndex);
  }

  return sample;
}

/*
 * Sample a volume for given pre-traversed voxel and domain offset (uniform).
 */
inline uniform float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const uniform uint64 &voxel,
                                       const uniform vec3ui &domainOffset,
                                       const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  if (vklVdbVoxelIsEmpty(voxel)) {
    return 0.f;
  }

  return VdbSampler_sample_inner(sampler, voxel, domainOffset, attributeIndex);
}

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
  const uniform VKLFormat leafFormat = vklVdbVoxelIsLeafPtr(voxel)
                                           ? vklVdbVoxelLeafGetFormat(voxel)
                                           : VKL_FORMAT_INVALID;
  const void *uniform leafPtr        = vklVdbVoxelLeafGetPtr(voxel);
  const uniform float background     = 0.f;
  uniform box1f valueRange           = make_box1f(background, background);

  switch (leafFormat) {
  case VKL_FORMAT_TILE:
    valueRange.lower = VdbSampler_sampleTileFloatLeaf(
        ((const Data1D *uniform)leafPtr), attributeIndex);
    valueRange.upper = valueRange.lower;
    break;
  case VKL_FORMAT_CONSTANT_ZYX:
    valueRange = make_box1f(pos_inf, neg_inf);
    if (grid->allLeavesCompact) {
      for (uniform unsigned int x = rangeX.x; x < rangeX.y; ++x) {
        for (uniform unsigned int y = rangeY.x; y < rangeY.y; ++y) {
          for (uniform unsigned int z = rangeZ.x; z < rangeZ.y; ++z) {
            const uniform float sample =
                VdbSampler_sampleConstantFloatLeafCompact(
                    ((const uniform Data1D *uniform)leafPtr),
                    make_vec3ui(x, y, z),
                    attributeIndex);
            extend(valueRange, sample);
          }
        }
      }
    } else {
      for (uniform unsigned int x = rangeX.x; x < rangeX.y; ++x) {
        for (uniform unsigned int y = rangeY.x; y < rangeY.y; ++y) {
          for (uniform unsigned int z = rangeZ.x; z < rangeZ.y; ++z) {
            const uniform float sample =
                VdbSampler_sampleConstantFloatLeafStrided(
                    ((const uniform Data1D *uniform)leafPtr),
                    make_vec3ui(x, y, z),
                    attributeIndex);
            extend(valueRange, sample);
          }
        }
      }
    }
    break;
  default: 
    break;
  }

  return valueRange;
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

  // We need to consider voxels outside this node in the value range because of
  // reconstruction filter support.
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
    make_vec2ui(0, 2)
  };

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
        // This is the offset of the current node relative to the central node.
        const uniform vec3i nodeOffset =
            make_vec3i((ix - 1) * VKL_VDB_RES_LEAF,
                       (iy - 1) * VKL_VDB_RES_LEAF,
                       (iz - 1) * VKL_VDB_RES_LEAF);

        // *centerNodeOffset is relative to the rootOrigin, but 
        // VdbSampler_traverse expects an index space coordinate. 
        const uniform vec3i nodeOrigin = make_vec3i(
          grid->rootOrigin.x + centerNodeOffset->x + nodeOffset.x,
          grid->rootOrigin.y + centerNodeOffset->y + nodeOffset.y,
          grid->rootOrigin.z + centerNodeOffset->z + nodeOffset.z
        );

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

// ---------------------------------------------------------------------------
// Interpolation.
// ---------------------------------------------------------------------------

/*
 * Nearest neighbor interpolation is the fastest version, but also gives
 * blocky results. This should be good for indirect light etc.
 */
inline float VdbSampler_interpolateNearest_varying(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  return VdbSampler_sample(sampler, ic, attributeIndex);
}

inline void VdbSampler_interpolateNearest_varying(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uint64 voxel;
  vec3ui domainOffset;
  VdbSampler_traverse(sampler, ic, voxel, domainOffset);

  for (uniform unsigned int a = 0; a < M; a++) {
    float samplesA =
        VdbSampler_sample(sampler, voxel, domainOffset, attributeIndices[a]);

    samples[a * VKL_TARGET_WIDTH + programIndex] = samplesA;
  }
}

inline uniform float VdbSampler_interpolateNearest_uniform(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  return VdbSampler_sample(sampler, ic, attributeIndex);
}

inline void VdbSampler_interpolateNearest_uniform(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform uint64 voxel;
  uniform vec3ui domainOffset;
  VdbSampler_traverse(sampler, ic, voxel, domainOffset);

  for (uniform unsigned int a = 0; a < M; a++) {
    float samplesA =
        VdbSampler_sample(sampler, voxel, domainOffset, attributeIndices[a]);

    samples[a] = extract(samplesA, 0);
  }
}

inline void VdbSampler_interpolateNearest_stream(
    const VdbSampler *uniform sampler,
    const uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                floor(indexCoordinates.y),
                                floor(indexCoordinates.z));

    uint64 voxel;
    vec3ui domainOffset;
    VdbSampler_traverse(sampler, ic, voxel, domainOffset);

    for (uniform unsigned int a = 0; a < M; a++) {
      float samplesA =
          VdbSampler_sample(sampler, voxel, domainOffset, attributeIndices[a]);

      samples[i * M + a] = samplesA;
    }
  }
}

/*
 * Compute voxel values for the eight corners required in trilinear
 * interpolation.
 * This is used for both sampling and gradient computation!
 */
inline void VdbSampler_computeVoxelValuesTrilinear(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
    const uniform uint32 attributeIndex,
    float *uniform sample)  // Array of VKL_TARGET_WIDTH * 8 elements!
{
  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  // The goal of this code is to keep as many lanes busy as possible.
  // The first case is that we have as many queries as there are
  // lanes, so we need not do anything smart (=expensive), no lane will
  // be idle.
  if (lanemask() == ((1 << VKL_TARGET_WIDTH) - 1)) {
    for (uniform unsigned int i = 0; i < 8; ++i) {
      const vec3i coord = ic + offset[i];
      sample[i * VKL_TARGET_WIDTH + programIndex] =
          VdbSampler_sample(sampler, coord, attributeIndex);
    }
  } else {
    // The opposite extreme is a single query. We perform as many of the
    // 8 lookups required for trilinear filtering in parallel as possible.
    // reduce_equal is a good way to get two pieces of information at
    // the same time, a) is there only one active instance? b) which one is it?
    uniform uint32 activeInstance;
    if (reduce_equal(programIndex, &activeInstance)) {
      const uniform vec3i iic = make_vec3i(extract(ic.x, activeInstance),
                                           extract(ic.y, activeInstance),
                                           extract(ic.z, activeInstance));
      foreach (o = 0 ... 8) {
        const vec3i coord = make_vec3i(
            iic.x + offset[o].x, iic.y + offset[o].y, iic.z + offset[o].z);
        sample[o * VKL_TARGET_WIDTH + activeInstance] =
            VdbSampler_sample(sampler, coord, attributeIndex);
      }
    }
    // Finally, a hybrid version: There are more than one but fewer than
    // VKL_TARGET_WIDTH queries. We may benefit from calling
    // VdbSampler_sample for multiple query points at once.
    else {
      // ith element is the index of the ith active lane.
      // packed_store_active2 may store an additional element, but we know
      // that at least one lane is inactive.
      uniform int progIdx[VKL_TARGET_WIDTH];
      const uniform int numActive = packed_store_active2(progIdx, programIndex);

      foreach_tiled(i = 0 ... numActive, o = 0 ... 8)
      {
        const int instance = progIdx[i];
        const vec3i iic    = make_vec3i(shuffle(ic.x, instance),
                                     shuffle(ic.y, instance),
                                     shuffle(ic.z, instance));
        const vec3i coord  = make_vec3i(
            iic.x + offset[o].x, iic.y + offset[o].y, iic.z + offset[o].z);
        sample[o * VKL_TARGET_WIDTH + instance] =
            VdbSampler_sample(sampler, coord, attributeIndex);
      }
    }
  }
}

/*
 * Traverse for the eight corners required in trilinear interpolation.
 */
inline void VdbSampler_traverseVoxelValuesTrilinear(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
    uint64 *uniform voxel,         // Array of VKL_TARGET_WIDTH * 8 elements!
    vec3ui *uniform domainOffset)  // Array of VKL_TARGET_WIDTH * 8 elements!
{
  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  // This approach modeled after the above
  // VdbSampler_computeVoxelValuesTrilinear(); see comments there for details.
  if (lanemask() == ((1 << VKL_TARGET_WIDTH) - 1)) {
    for (uniform unsigned int i = 0; i < 8; ++i) {
      const vec3i coord = ic + offset[i];

      varying uint64 voxelV;
      varying vec3ui domainOffsetV;
      VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

      voxel[i * VKL_TARGET_WIDTH + programIndex]        = voxelV;
      domainOffset[i * VKL_TARGET_WIDTH + programIndex] = domainOffsetV;
    }
  } else {
    uniform uint32 activeInstance;
    if (reduce_equal(programIndex, &activeInstance)) {
      const uniform vec3i iic = make_vec3i(extract(ic.x, activeInstance),
                                           extract(ic.y, activeInstance),
                                           extract(ic.z, activeInstance));
      foreach (o = 0 ... 8) {
        const vec3i coord = make_vec3i(
            iic.x + offset[o].x, iic.y + offset[o].y, iic.z + offset[o].z);

        varying uint64 voxelV;
        varying vec3ui domainOffsetV;
        VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

        voxel[o * VKL_TARGET_WIDTH + activeInstance]        = voxelV;
        domainOffset[o * VKL_TARGET_WIDTH + activeInstance] = domainOffsetV;
      }
    } else {
      uniform int progIdx[VKL_TARGET_WIDTH];
      const uniform int numActive = packed_store_active2(progIdx, programIndex);

      foreach_tiled(i = 0 ... numActive, o = 0 ... 8)
      {
        const int instance = progIdx[i];
        const vec3i iic    = make_vec3i(shuffle(ic.x, instance),
                                     shuffle(ic.y, instance),
                                     shuffle(ic.z, instance));
        const vec3i coord  = make_vec3i(
            iic.x + offset[o].x, iic.y + offset[o].y, iic.z + offset[o].z);

        varying uint64 voxelV;
        varying vec3ui domainOffsetV;
        VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

        voxel[o * VKL_TARGET_WIDTH + instance]        = voxelV;
        domainOffset[o * VKL_TARGET_WIDTH + instance] = domainOffsetV;
      }
    }
  }
}

/*
 * Compute voxel values for the eight corners required in trilinear
 * interpolation for given pre-traversed voxel and domain offset.
 */
inline void VdbSampler_computeVoxelValuesTrilinear(
    const VdbSampler *uniform sampler,
    const uint64 *uniform voxel,  // Array of VKL_TARGET_WIDTH * 8 elements!
    const vec3ui *uniform
        domainOffset,  // Array of VKL_TARGET_WIDTH * 8 elements!
    const uniform uint32 attributeIndex,
    float *uniform sample)  // Array of VKL_TARGET_WIDTH * 8 elements!
{
  // This approach modeled after the above
  // VdbSampler_computeVoxelValuesTrilinear(); see comments there for details.
  if (lanemask() == ((1 << VKL_TARGET_WIDTH) - 1)) {
    for (uniform unsigned int i = 0; i < 8; ++i) {
      const varying uint64 voxelV = voxel[i * VKL_TARGET_WIDTH + programIndex];
      const varying vec3ui domainOffsetV =
          domainOffset[i * VKL_TARGET_WIDTH + programIndex];

      sample[i * VKL_TARGET_WIDTH + programIndex] =
          VdbSampler_sample(sampler, voxelV, domainOffsetV, attributeIndex);
    }
  } else {
    uniform uint32 activeInstance;
    if (reduce_equal(programIndex, &activeInstance)) {
      foreach (o = 0 ... 8) {
        const varying uint64 voxelV =
            voxel[o * VKL_TARGET_WIDTH + activeInstance];
        const varying vec3ui domainOffsetV =
            domainOffset[o * VKL_TARGET_WIDTH + activeInstance];

        sample[o * VKL_TARGET_WIDTH + activeInstance] =
            VdbSampler_sample(sampler, voxelV, domainOffsetV, attributeIndex);
      }
    } else {
      uniform int progIdx[VKL_TARGET_WIDTH];
      const uniform int numActive = packed_store_active2(progIdx, programIndex);

      foreach_tiled(i = 0 ... numActive, o = 0 ... 8)
      {
        const int instance = progIdx[i];

        const varying uint64 voxelV = voxel[o * VKL_TARGET_WIDTH + instance];
        const varying vec3ui domainOffsetV =
            domainOffset[o * VKL_TARGET_WIDTH + instance];

        sample[o * VKL_TARGET_WIDTH + instance] =
            VdbSampler_sample(sampler, voxelV, domainOffsetV, attributeIndex);
      }
    }
  }
}

/*
 * Trilinear sampling is a good default for directly visible volumes.
 * The implementation is optimized to exploit SIMD.
 */
inline varying float VdbSampler_interpolateTrilinear_varying(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(sampler, ic, attributeIndex, sample);

  const varying float *uniform s = (const varying float *uniform) & sample;
  return lerp(
      delta.x,
      lerp(delta.y, lerp(delta.z, s[0], s[1]), lerp(delta.z, s[2], s[3])),
      lerp(delta.y, lerp(delta.z, s[4], s[5]), lerp(delta.z, s[6], s[7])));
}

inline void VdbSampler_interpolateTrilinear_varying(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);

  uniform uint64 voxel[VKL_TARGET_WIDTH * 8];
  uniform vec3ui domainOffset[VKL_TARGET_WIDTH * 8];
  VdbSampler_traverseVoxelValuesTrilinear(sampler, ic, voxel, domainOffset);

  for (uniform unsigned int a = 0; a < M; a++) {
    uniform float sample[VKL_TARGET_WIDTH * 8];
    VdbSampler_computeVoxelValuesTrilinear(
        sampler, voxel, domainOffset, attributeIndices[a], sample);

    const varying float *uniform s = (const varying float *uniform) & sample;
    varying float samplesA         = lerp(
        delta.x,
        lerp(delta.y, lerp(delta.z, s[0], s[1]), lerp(delta.z, s[2], s[3])),
        lerp(delta.y, lerp(delta.z, s[4], s[5]), lerp(delta.z, s[6], s[7])));

    samples[a * VKL_TARGET_WIDTH + programIndex] = samplesA;
  }
}

/*
 * Uniform path. This allows us to skip the selection magic in the function
 * above if we know that there is only one query.
 */
inline uniform float VdbSampler_interpolateTrilinear_uniform(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  const uniform vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));
  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);

  unmasked
  {
    uniform float sample[8];
    foreach (o = 0 ... 8) {
      const vec3i coord = make_vec3i(
          ic.x + offset[o].x, ic.y + offset[o].y, ic.z + offset[o].z);
      sample[o] = VdbSampler_sample(sampler, coord, attributeIndex);
    }

    return lerp(delta.x,
                lerp(delta.y,
                     lerp(delta.z, sample[0], sample[1]),
                     lerp(delta.z, sample[2], sample[3])),
                lerp(delta.y,
                     lerp(delta.z, sample[4], sample[5]),
                     lerp(delta.z, sample[6], sample[7])));
  }
}

inline void VdbSampler_interpolateTrilinear_uniform(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  const uniform vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));
  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);

  unmasked
  {
    uniform uint64 voxel[8];
    uniform vec3ui domainOffset[8];

    foreach (o = 0 ... 8) {
      const vec3i coord = make_vec3i(
          ic.x + offset[o].x, ic.y + offset[o].y, ic.z + offset[o].z);

      varying uint64 voxelV        = voxel[o];
      varying vec3ui domainOffsetV = domainOffset[o];
      VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

      voxel[o]        = voxelV;
      domainOffset[o] = domainOffsetV;
    }

    for (uniform unsigned int a = 0; a < M; a++) {
      uniform float sample[8];

      foreach (o = 0 ... 8) {
        const varying uint64 voxelV        = voxel[o];
        const varying vec3ui domainOffsetV = domainOffset[o];

        sample[o] = VdbSampler_sample(
            sampler, voxelV, domainOffsetV, attributeIndices[a]);
      }

      samples[a] = lerp(delta.x,
                        lerp(delta.y,
                             lerp(delta.z, sample[0], sample[1]),
                             lerp(delta.z, sample[2], sample[3])),
                        lerp(delta.y,
                             lerp(delta.z, sample[4], sample[5]),
                             lerp(delta.z, sample[6], sample[7])));
    }
  }
}

inline void VdbSampler_interpolateTrilinear_stream(
    const VdbSampler *uniform sampler,
    const uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                floor(indexCoordinates.y),
                                floor(indexCoordinates.z));
    const vec3f delta = indexCoordinates - make_vec3f(ic);

    uniform uint64 voxel[VKL_TARGET_WIDTH * 8];
    uniform vec3ui domainOffset[VKL_TARGET_WIDTH * 8];
    VdbSampler_traverseVoxelValuesTrilinear(sampler, ic, voxel, domainOffset);

    for (uniform unsigned int a = 0; a < M; a++) {
      uniform float sample[VKL_TARGET_WIDTH * 8];
      VdbSampler_computeVoxelValuesTrilinear(
          sampler, voxel, domainOffset, attributeIndices[a], sample);

      const varying float *uniform s = (const varying float *uniform) & sample;
      varying float samplesA         = lerp(
          delta.x,
          lerp(delta.y, lerp(delta.z, s[0], s[1]), lerp(delta.z, s[2], s[3])),
          lerp(delta.y, lerp(delta.z, s[4], s[5]), lerp(delta.z, s[6], s[7])));

      samples[i * M + a] = samplesA;
    }
  }
}

/*
 * Gradients in piecewise constant fields are zero (almost everywhere, we'll
 * say everywhere...)
 */
inline vec3f VdbSampler_computeGradientNearest(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  return make_vec3f(0.f);
}

/*
 * Gradients in trilinear fields.
 */
inline vec3f VdbSampler_computeGradientTrilinear(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const uniform uint32 attributeIndex)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(sampler, ic, attributeIndex, sample);

  const varying float *uniform s = (const varying float *uniform) & sample;

  vec3f gradient;
  gradient.x = lerp(delta.y,
                    lerp(delta.z, s[4] - s[0], s[5] - s[1]),
                    lerp(delta.z, s[6] - s[2], s[7] - s[3]));
  gradient.y = lerp(delta.x,
                    lerp(delta.z, s[2] - s[0], s[3] - s[1]),
                    lerp(delta.z, s[6] - s[4], s[7] - s[5]));
  gradient.z = lerp(delta.x,
                    lerp(delta.y, s[1] - s[0], s[3] - s[2]),
                    lerp(delta.y, s[5] - s[4], s[7] - s[6]));
  return gradient;
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

/*
 * Special case: we know that coordinates are uniform.
 */
export void EXPORT_UNIQUE(VdbSampler_computeSample_uniform,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  float *uniform sample = (float *uniform)_samples;

  const uniform vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

  switch (sampler->super.filter) {
  case VKL_FILTER_TRILINEAR:
    *sample = VdbSampler_interpolateTrilinear_uniform(
        sampler, indexCoordinates, attributeIndex);
    break;
  default:
    *sample = VdbSampler_interpolateNearest_uniform(
        sampler, indexCoordinates, attributeIndex);
    break;
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSample,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _samples)
{
  if (imask[programIndex]) {
    const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
    assert(sampler);
    assert(sampler->grid);

    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    switch (sampler->super.filter) {
    case VKL_FILTER_TRILINEAR:
      *samples = VdbSampler_interpolateTrilinear_varying(
          sampler, indexCoordinates, attributeIndex);
      break;
    default:
      *samples = VdbSampler_interpolateNearest_varying(
          sampler, indexCoordinates, attributeIndex);
      break;
    }
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM_uniform,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;

  const uniform vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

  switch (sampler->super.filter) {
  case VKL_FILTER_TRILINEAR:
    VdbSampler_interpolateTrilinear_uniform(
        sampler, indexCoordinates, M, attributeIndices, samples);
    break;
  default:
    VdbSampler_interpolateNearest_uniform(
        sampler, indexCoordinates, M, attributeIndices, samples);
    break;
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          float *uniform samples)
{
  if (imask[programIndex]) {
    const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
    assert(sampler);
    assert(sampler->grid);

    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;

    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    switch (sampler->super.filter) {
    case VKL_FILTER_TRILINEAR:
      VdbSampler_interpolateTrilinear_varying(
          sampler, indexCoordinates, M, attributeIndices, samples);
      break;
    default:
      VdbSampler_interpolateNearest_varying(
          sampler, indexCoordinates, M, attributeIndices, samples);
      break;
    }
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSample_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    switch (sampler->super.filter) {
    case VKL_FILTER_TRILINEAR:
      samples[i] = VdbSampler_interpolateTrilinear_varying(
          sampler, indexCoordinates, attributeIndex);
      break;
    default:
      samples[i] = VdbSampler_interpolateNearest_varying(
          sampler, indexCoordinates, attributeIndex);
      break;
    }
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  switch (sampler->super.filter) {
  case VKL_FILTER_TRILINEAR:
    VdbSampler_interpolateTrilinear_stream(
        sampler, N, objectCoordinates, M, attributeIndices, samples);
    break;
  default:
    VdbSampler_interpolateNearest_stream(
        sampler, N, objectCoordinates, M, attributeIndices, samples);
    break;
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _gradients)
{
  if (imask[programIndex]) {
    const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
    assert(sampler);
    assert(sampler->grid);

    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    vec3f gradient;
    switch (sampler->super.gradientFilter) {
    case VKL_FILTER_TRILINEAR:
      gradient = VdbSampler_computeGradientTrilinear(
          sampler, indexCoordinates, attributeIndex);
      break;
    default:
      gradient = VdbSampler_computeGradientNearest(
          sampler, indexCoordinates, attributeIndex);
      break;
    }

    // Note: xfmNormal takes inverse!
    *gradients = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          vec3f *uniform gradients)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    vec3f gradient;
    switch (sampler->super.gradientFilter) {
    case VKL_FILTER_TRILINEAR:
      gradient = VdbSampler_computeGradientTrilinear(
          sampler, indexCoordinates, attributeIndex);
      break;
    default:
      gradient = VdbSampler_computeGradientNearest(
          sampler, indexCoordinates, attributeIndex);
      break;
    }

    // Note: xfmNormal takes inverse!
    gradients[i] = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

// -----------------------------------------------------------------------------
// Interface for iterators
// -----------------------------------------------------------------------------

inline float VdbSampler_iterator_computeSample_varying(
    const Sampler *uniform _sampler,
    const varying vec3f &objectCoordinates,
    const varying float &_time)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, objectCoordinates);

  const uniform uint32 attributeIndex = 0;

  float sample = 0.f;
  switch (sampler->super.filter) {
  case VKL_FILTER_TRILINEAR:
    sample = VdbSampler_interpolateTrilinear_varying(
        sampler, indexCoordinates, attributeIndex);
    break;
  default:
    sample = VdbSampler_interpolateNearest_varying(
        sampler, indexCoordinates, attributeIndex);
    break;
  }

  return sample;
}

// -----------------------------------------------------------------------------

export VdbSampler *uniform
EXPORT_UNIQUE(VdbSampler_create,
              const void *uniform _volume,
              const void *uniform leafAccessObservers)
{
  VdbSampler *uniform sampler = uniform new VdbSampler;
  memset(sampler, 0, sizeof(uniform VdbSampler));

  // Generic sampler interface - this is used by iterators.
  const VdbVolume *uniform volume = (const VdbVolume *uniform)_volume;
  sampler->super.volume           = &volume->super;

  // Our internal sampling interface. The sampler object is passed into the
  // inner loop.
  const VdbVolume *uniform vdbVolume = (const VdbVolume *uniform)volume;
  sampler->grid                      = volume->grid;
  sampler->leafAccessObservers       = leafAccessObservers;
  return sampler;
}

export void EXPORT_UNIQUE(VdbSampler_set,
                          void *uniform _sampler,
                          uniform VKLFilter filter,
                          uniform VKLFilter gradientFilter,
                          uniform vkl_uint32 maxSamplingDepth,
                          uniform vkl_uint32 maxIteratorDepth)
{
  VdbSampler *uniform sampler = (VdbSampler * uniform) _sampler;
  CALL_ISPC(Sampler_setFilters, &sampler->super, filter, gradientFilter);

  // For hit iterators.
  sampler->super.computeSample_varying =
      VdbSampler_iterator_computeSample_varying;

  sampler->maxSamplingDepth = maxSamplingDepth;
  sampler->maxIteratorDepth = maxIteratorDepth;
}

export void EXPORT_UNIQUE(VdbSampler_destroy, void *uniform _sampler)
{
  VdbSampler *uniform sampler = (VdbSampler * uniform) _sampler;
  delete sampler;
}

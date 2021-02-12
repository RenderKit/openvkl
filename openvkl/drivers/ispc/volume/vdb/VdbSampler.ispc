// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "../common/Data.ih"
#include "VdbLeafAccessObserver.ih"
#include "VdbSampleConstantLeaf.ih"
#include "VdbSampleTileLeaf.ih"
#include "VdbSampler.ih"
#include "VdbSampler_stencilDispatch.ih"
#include "VdbVolume.ih"
#include "common/export_util.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.ih"

// -----------------------------------------------------------------------------
// Filter stencils.
// -----------------------------------------------------------------------------

#define VKL_STENCIL_TRILINEAR_SIZE    8
#define VKL_STENCIL_TRILINEAR_OFFSETS {{0, 0, 0},                              \
                                       {0, 0, 1},                              \
                                       {0, 1, 0},                              \
                                       {0, 1, 1},                              \
                                       {1, 0, 0},                              \
                                       {1, 0, 1},                              \
                                       {1, 1, 0},                              \
                                       {1, 1, 1}}

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

inline void extendValueRangeFilterFloat(const void *uniform _grid,
                                        const vec3ui *uniform offset,
                                        uniform uint32 level,
                                        uniform uint32 attributeIndex,
                                        uniform box1f *uniform range)
{
  uniform VdbSampler sampler;
  memset(&sampler, 0, sizeof(uniform VdbSampler));
  sampler.grid             = (const VdbGrid *uniform)_grid;
  sampler.maxSamplingDepth = VKL_VDB_NUM_LEVELS - 1;
  // We set none of the other members as the internal code does not require
  // them.

  float vminFilter            = pos_inf;
  float vmaxFilter            = neg_inf;
  const uniform uint32 res    = vklVdbLevelRes(level);
  const VdbGrid *uniform grid = (const VdbGrid *uniform)_grid;
  const uniform vec3ui os     = *offset;
  const uniform int radius    = 1;  // TRILINEAR INTERPOLATION!
  // We compute min/max over the voxels (one filter radius) after upper bound.
  // These are voxels that can be interpolated into our node, so we must include
  // them for conservative value ranges.
  foreach (z = -radius... res + radius, y = -radius... res + radius) {
    const int32 x      = res;
    const float sample = VdbSampler_sample(
        &sampler, make_vec3i(x + os.x, y + os.y, z + os.z), attributeIndex);
    vminFilter = min(vminFilter, sample);
    vmaxFilter = max(vmaxFilter, sample);
  }
  // We only have to go to x = res+radius as we added those voxels above.
  foreach (z = -radius... res + radius, x = -radius... res) {
    const int32 y      = res;
    const float sample = VdbSampler_sample(
        &sampler, make_vec3i(x + os.x, y + os.y, z + os.z), attributeIndex);
    vminFilter = min(vminFilter, sample);
    vmaxFilter = max(vmaxFilter, sample);
  }
  // We only have to go to x = res+radius and y = res+radius as we added those
  // voxels above.
  foreach (y = -radius... res, x = -radius... res) {
    const int32 z      = res;
    const float sample = VdbSampler_sample(
        &sampler, make_vec3i(x + os.x, y + os.y, z + os.z), attributeIndex);
    vminFilter = min(vminFilter, sample);
    vmaxFilter = max(vmaxFilter, sample);
  }

  range->lower = min(reduce_min(vminFilter), range->lower);
  range->upper = max(reduce_max(vmaxFilter), range->upper);
}

/*
 * Compute the value range on the given tile
 */
export void EXPORT_UNIQUE(VdbSampler_valueRangeTileFloat,
                          const void *uniform _grid,
                          const vec3ui *uniform offset,
                          uniform uint32 level,
                          uniform uint32 attributeIndex,
                          uniform box1f *uniform range)
{
  range->lower = pos_inf;
  range->upper = neg_inf;
  extendValueRangeFilterFloat(_grid, offset, level, attributeIndex, range);
}

/*
 * Compute the value range on the given constant float leaf.
 */
export void EXPORT_UNIQUE(VdbSampler_valueRangeConstantFloat,
                          const void *uniform _grid,
                          const vec3ui *uniform offset,
                          uniform uint32 level,
                          uniform uint32 attributeIndex,
                          uniform box1f *uniform range)
{
  range->lower = pos_inf;
  range->upper = neg_inf;
  extendValueRangeFilterFloat(_grid, offset, level, attributeIndex, range);
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
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, {
      sample[tgtIdx] = VdbSampler_sample(sampler, coord, attributeIndex);
  });
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
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, {
      uint64 voxelV;
      vec3ui domainOffsetV;
      VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

      voxel[tgtIdx]        = voxelV;
      domainOffset[tgtIdx] = domainOffsetV;
  });
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
  // TODO: We might be able to avoid this, but it will complicate the macro.
  const vec3i ic = {0,0,0};
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, {
      const uint64 voxelV = voxel[tgtIdx];
      const vec3ui domainOffsetV = domainOffset[tgtIdx];

      sample[tgtIdx] =
          VdbSampler_sample(sampler, voxelV, domainOffsetV, attributeIndex);
  });
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
  const uniform vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform float sample[8];

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, {
    sample[tgtIdx] = VdbSampler_sample(sampler, coord, attributeIndex);
  })

  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);
  return lerp(delta.x,
              lerp(delta.y,
                   lerp(delta.z, sample[0], sample[1]),
                   lerp(delta.z, sample[2], sample[3])),
              lerp(delta.y,
                   lerp(delta.z, sample[4], sample[5]),
                   lerp(delta.z, sample[6], sample[7])));
}

inline void VdbSampler_interpolateTrilinear_uniform(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  const uniform vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));
  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);

  uniform uint64 voxel[8];
  uniform vec3ui domainOffset[8];

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, {
    // TODO: Why is this necessary?
    varying uint64 voxelV        = voxel[tgtIdx];
    varying vec3ui domainOffsetV = domainOffset[tgtIdx];

    VdbSampler_traverse(sampler, coord, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  })

  for (uniform unsigned int a = 0; a < M; a++) {
    uniform float sample[8];

    __vkl_stencil_dispatch_uniform(TRILINEAR, ic, {
      const varying uint64 voxelV        = voxel[tgtIdx];
      const varying vec3ui domainOffsetV = domainOffset[tgtIdx];
      sample[tgtIdx] = VdbSampler_sample(
          sampler, voxelV, domainOffsetV, attributeIndices[a]);
    })

    samples[a] = lerp(delta.x,
                      lerp(delta.y,
                           lerp(delta.z, sample[0], sample[1]),
                           lerp(delta.z, sample[2], sample[3])),
                      lerp(delta.y,
                           lerp(delta.z, sample[4], sample[5]),
                           lerp(delta.z, sample[6], sample[7])));
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

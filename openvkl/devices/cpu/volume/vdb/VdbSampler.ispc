// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "../common/Data.ih"
#include "VdbLeafAccessObserver.ih"
#include "VdbSampler.ih"
#include "VdbSampler_denseZYX.ih"
#include "VdbSampler_stencilDispatch.ih"
#include "VdbSampler_tile.ih"
#include "VdbVolume.ih"
#include "common/export_util.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.ih"

// -----------------------------------------------------------------------------
// VdbSampler implements special code paths for the following scenarios:
//
// - single attribute uniform query
// - single attribute varying query
// - multi attribute uniform query
// - multi attribute varying query
//
// In addition, we provide single and multi attribute stream queries, which
// are implemented using the varying code path.
//
// We support NEAREST and TRILINEAR filter modes.
//
// VdbSampler follows this basic strategy:
//
// 1) Transform query coordinates to index space.
// 2) For each entry in the filter stencil, *traverse* the data structure to
//    find the corresponding voxel code and domainOffset.
// 3) For each entry in the filter stencil, sample the volume using the voxel
//    code and domain offset.
// 4) Interpolate to obtain the final sample.
// -----------------------------------------------------------------------------

/*
 * This file forces a lot of inlining for maximum run time performance, which
 * can lead to excessive compile times.
 *
 * The macro below creates a noinline barrier for functions in
 * VdbSamplerDispatchInner.ih. This can improve compile times significantly.
 *
 * The definition should be commented out again before merging.
 */

#if defined(VDB_SAMPLER_DISPATCH_INNER_NOINLINE)
#warning "VDB_SAMPLER_DISPATCH_INNER_NOINLINE is set."

noinline void VdbSampler_dispatchInner_uniform_uniform(
    const VdbSampler *uniform sampler,
    const uniform uint64 nodeIndex,
    const uniform vec3ui &domainOffset,
    uniform uint64 &voxel)
{
  VdbSampler_dispatchInner_uniform_uniform_0(
      sampler, nodeIndex, domainOffset, voxel);
}

noinline void VdbSampler_dispatchInner_uniform_varying(
    const VdbSampler *uniform sampler,
    const uniform uint64 nodeIndex,
    const vec3ui &domainOffset,
    varying uint64 &voxel)
{
  VdbSampler_dispatchInner_uniform_varying_0(
      sampler, nodeIndex, domainOffset, voxel);
}

noinline void VdbSampler_dispatchInner_varying_varying(
    const VdbSampler *uniform sampler,
    const uint64 nodeIndex,
    const vec3ui &domainOffset,
    uint64 &voxel)
{
  VdbSampler_dispatchInner_varying_varying_0(
      sampler, nodeIndex, domainOffset, voxel);
}

#else

#define VdbSampler_dispatchInner_uniform_uniform \
  VdbSampler_dispatchInner_uniform_uniform_0

#define VdbSampler_dispatchInner_uniform_varying \
  VdbSampler_dispatchInner_uniform_varying_0

#define VdbSampler_dispatchInner_varying_varying \
  VdbSampler_dispatchInner_varying_varying_0

#endif

// ---------------------------------------------------------------------------
// Generate code to dispatch to a leaf handler implementation based on the
// attribute data type, format, and temporal format.
// ---------------------------------------------------------------------------

#define __vkl_leaf_handler_impl(handler, postfix, ...) \
  handler##_##postfix(__VA_ARGS__);

#define __vkl_leaf_handler_format(handler, leafFormat, postfix, ...)  \
  if (leafFormat == VKL_FORMAT_TILE) {                                \
    __vkl_leaf_handler_impl(handler, tile_##postfix, __VA_ARGS__)     \
  } else if (leafFormat == VKL_FORMAT_DENSE_ZYX) {                    \
    __vkl_leaf_handler_impl(handler, denseZYX_##postfix, __VA_ARGS__) \
  } else {                                                            \
    assert(false);                                                    \
  }

#define __vkl_leaf_handler_temporalFormat(handler, leafFormat, postfix, ...) \
  if (leafTemporalFormat == VKL_TEMPORAL_FORMAT_CONSTANT) {                  \
    __vkl_leaf_handler_format(                                               \
        handler, leafFormat, constant_##postfix, __VA_ARGS__)                \
  } else if (leafTemporalFormat == VKL_TEMPORAL_FORMAT_STRUCTURED) {         \
    __vkl_leaf_handler_format(                                               \
        handler, leafFormat, structured_##postfix, __VA_ARGS__)              \
  } else if (leafTemporalFormat == VKL_TEMPORAL_FORMAT_UNSTRUCTURED) {       \
    __vkl_leaf_handler_format(                                               \
        handler, leafFormat, unstructured_##postfix, __VA_ARGS__)            \
  } else {                                                                   \
    assert(false);                                                           \
  }

#define __vkl_leaf_handler(                                                    \
    handler, dataType, leafFormat, leafTemporalFormat, ...)                    \
  if (dataType == VKL_HALF) {                                                  \
    __vkl_leaf_handler_temporalFormat(handler, leafFormat, half, __VA_ARGS__)  \
  } else if (dataType == VKL_FLOAT) {                                          \
    __vkl_leaf_handler_temporalFormat(handler, leafFormat, float, __VA_ARGS__) \
  } else {                                                                     \
    assert(false);                                                             \
  }

// ---------------------------------------------------------------------------
// Find the origin of the leaf node containing domainOffset.
// Note: Performance is significantly better when these functions return
//       the leaf origin as opposed to having a reference parameter.
// ---------------------------------------------------------------------------

inline uniform vec3ui
VdbSampler_toLeafOrigin(const uniform vec3ui &domainOffset)
{
  static const uniform uint32 originMask =
      ~(((uniform uint32)(VKL_VDB_RES_LEAF - 1)));
  return make_vec3ui(domainOffset.x & originMask,
                     domainOffset.y & originMask,
                     domainOffset.z & originMask);
}

inline vec3ui VdbSampler_toLeafOrigin(const vec3ui &domainOffset)
{
  static const uniform uint32 originMask =
      ~(((uniform uint32)(VKL_VDB_RES_LEAF - 1)));
  return make_vec3ui(domainOffset.x & originMask,
                     domainOffset.y & originMask,
                     domainOffset.z & originMask);
}

// ---------------------------------------------------------------------------
// Map the given index space coordinate to a (root-relative) domain offset.
// Note that we do not check if the ic-rootOrigin is positive here; instead
// we rely on the fact that negative numbers in the subtraction below will be
// mapped very large values in the conversion.
// ---------------------------------------------------------------------------

inline uniform vec3ui VdbSampler_toDomainOffset(const uniform vec3i &indexCoord,
                                                const uniform vec3i &rootOrigin)
{
  return make_vec3ui(indexCoord.x - rootOrigin.x,
                     indexCoord.y - rootOrigin.y,
                     indexCoord.z - rootOrigin.z);
}

inline vec3ui VdbSampler_toDomainOffset(const vec3i &indexCoord,
                                        const uniform vec3i &rootOrigin)
{
  return make_vec3ui(indexCoord.x - rootOrigin.x,
                     indexCoord.y - rootOrigin.y,
                     indexCoord.z - rootOrigin.z);
}

// ---------------------------------------------------------------------------

inline uniform bool VdbSampler_isInDomain(const uniform vec3ui &domainOffset)
{
  return domainOffset.x < VKL_VDB_RES_0 && domainOffset.y < VKL_VDB_RES_0 &&
         domainOffset.z < VKL_VDB_RES_0;
}

inline bool VdbSampler_isInDomain(const vec3ui &domainOffset)
{
  return domainOffset.x < VKL_VDB_RES_0 && domainOffset.y < VKL_VDB_RES_0 &&
         domainOffset.z < VKL_VDB_RES_0;
}

// ---------------------------------------------------------------------------
// Clip the index space coordinate to the grid bounding box, and traverse
// the hierarchy to find the correct voxel.
// ---------------------------------------------------------------------------

// Uniform traversal.
inline void VdbSampler_traverse(const VdbSampler *uniform sampler,
                                const uniform vec3i &ic,
                                uniform uint64 &voxel,
                                uniform vec3ui &domainOffset)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  voxel        = vklVdbVoxelMakeEmpty();
  domainOffset = VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);
  if (VdbSampler_isInDomain(domainOffset)) {
    const uniform vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);
    VdbSampler_dispatchInner_uniform_uniform(sampler, 0ul, leafOrigin, voxel);
  }
}

// Varying traversal.
inline void VdbSampler_traverse(const VdbSampler *uniform sampler,
                                const vec3i &ic,
                                uint64 &voxel,
                                vec3ui &domainOffset)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  voxel        = vklVdbVoxelMakeEmpty();
  domainOffset = VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);
  if (VdbSampler_isInDomain(domainOffset)) {
    const vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);
    uniform vec3ui uniformLeafOrigin;
    if (reduce_equal(leafOrigin.x, &uniformLeafOrigin.x) &&
        reduce_equal(leafOrigin.y, &uniformLeafOrigin.y) &&
        reduce_equal(leafOrigin.z, &uniformLeafOrigin.z)) {
      // All queries are in the same leaf node. Do uniform traversal!
      uniform uint64 voxelU = vklVdbVoxelMakeEmpty();
      VdbSampler_dispatchInner_uniform_uniform(
          sampler, 0ul, uniformLeafOrigin, voxelU);
      voxel = voxelU;
    } else {
      // Varying traversal.
      VdbSampler_dispatchInner_uniform_varying(sampler, 0ul, leafOrigin, voxel);
    }
  }
}

// ---------------------------------------------------------------------------
// Given a domain relative index space coordinate and a voxel code, sample the
// leaf node.
// ---------------------------------------------------------------------------

// These helper functions handle all legal combinations of sampling uniform /
// varying voxels at uniform / varying offsets, and are used just below.
#define template_VdbSampler_sample_inner(univaryVoxel, univaryOffset)               \
  inline univaryOffset float VdbSampler_sample_inner(                               \
      const VdbSampler *uniform sampler,                                            \
      const univaryVoxel uint64 &voxel,                                             \
      const univaryOffset vec3ui &domainOffset,                                     \
      const univaryOffset float &time,                                              \
      const uniform uint32 attributeIndex)                                          \
  {                                                                                 \
    univaryOffset float sample  = 0.f;                                              \
    const VdbGrid *uniform grid = sampler->grid;                                    \
                                                                                    \
    if (vklVdbVoxelIsLeafPtr(voxel)) {                                              \
      const uniform vkl_uint32 voxelType =                                          \
          sampler->grid->attributeTypes[attributeIndex];                            \
      const univaryVoxel VKLFormat leafFormat =                                     \
          vklVdbVoxelLeafGetFormat(voxel);                                          \
      const univaryVoxel VKLTemporalFormat leafTemporalFormat =                     \
          vklVdbVoxelLeafGetTemporalFormat(voxel);                                  \
      const univaryVoxel uint64 leafIndex = vklVdbVoxelLeafGetIndex(voxel);         \
      const univaryVoxel uint64 leafDataIndex =                                     \
          vklVdbGetLeafDataIndex(grid, leafIndex, attributeIndex);                  \
      __vkl_leaf_handler(                                                           \
          sample = VdbSampler_sample_##univaryVoxel##_##univaryOffset,              \
          voxelType,                                                                \
          leafFormat,                                                               \
          leafTemporalFormat,                                                       \
          grid,                                                                     \
          leafIndex,                                                                \
          leafDataIndex,                                                            \
          domainOffset,                                                             \
          time)                                                                     \
    } else if (vklVdbVoxelIsError(voxel)) {                                         \
      univaryVoxel uint8 level;                                                     \
      univaryVoxel uint32 voxelOffset;                                              \
      vklVdbVoxelErrorGet(voxel, level, voxelOffset);                               \
      const univaryVoxel range1f valueRange =                                       \
          grid->levels[level]                                                       \
              .valueRange[voxelOffset * grid->numAttributes + attributeIndex];      \
      sample = 0.5f * (valueRange.lower + valueRange.upper);                        \
    }                                                                               \
    return sample;                                                                  \
  }

template_VdbSampler_sample_inner(varying, varying);
template_VdbSampler_sample_inner(uniform, varying);
template_VdbSampler_sample_inner(uniform, uniform);
#undef template_VdbSampler_sample_inner

// Uniform sampling.
// Note: This function may seem unnecessary in the uniform path. However,
// keeping this thin wrapper ensures that downstream code is as uniform as
// possible.
inline uniform float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const uniform uint64 &voxel,
                                       const uniform vec3ui &domainOffset,
                                       const uniform float time,
                                       const uniform uint32 attributeIndex)
{
  return VdbSampler_sample_inner(
      sampler, voxel, domainOffset, time, attributeIndex);
}

// Varying sampling.
inline float VdbSampler_sample(const VdbSampler *uniform sampler,
                               const uint64 &voxel,
                               const vec3ui &domainOffset,
                               const float &time,
                               const uniform uint32 attributeIndex)
{
  uniform uint64 uniformVoxel;
  if (reduce_equal(voxel, &uniformVoxel)) {
    return VdbSampler_sample_inner(
        sampler, uniformVoxel, domainOffset, time, attributeIndex);
  } else {
    return VdbSampler_sample_inner(
        sampler, voxel, domainOffset, time, attributeIndex);
  }
}

// ---------------------------------------------------------------------------
// Traverse and sample in a single call.
//
// This is generally more efficient than calling traverse() and sample()
// separately if only a single element needs to be looked up.
// ---------------------------------------------------------------------------

inline uniform float VdbSampler_traverseAndSample(
    const VdbSampler *uniform sampler,
    const uniform vec3i &ic,
    const uniform float time,
    const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const uniform vec3ui domainOffset =
      VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);
  if (VdbSampler_isInDomain(domainOffset)) {
    const uniform vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);

    uniform uint64 voxelU = vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_uniform(sampler, 0ul, leafOrigin, voxelU);
    return VdbSampler_sample_inner(
        sampler, voxelU, domainOffset, time, attributeIndex);
  }

  return 0.f;
}

inline uniform bool reduce_equal(const vec3ui &v, vec3ui *uniform uv)
{
  return reduce_equal(v.x, &(uv->x)) && reduce_equal(v.y, &(uv->y)) &&
         reduce_equal(v.z, &(uv->z));
}

inline float VdbSampler_traverseAndSample(const VdbSampler *uniform sampler,
                                          const vec3i &ic,
                                          const float &time,
                                          const uniform uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);

  const vec3ui domainOffset =
      VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);
  if (VdbSampler_isInDomain(domainOffset)) {
    const vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);
    uniform vec3ui uniformLeafOrigin;
    if (reduce_equal(leafOrigin, &uniformLeafOrigin)) {
      // All queries are in the same leaf node. Do uniform traversal!
      uniform uint64 voxelU = vklVdbVoxelMakeEmpty();
      VdbSampler_dispatchInner_uniform_uniform(
          sampler, 0ul, uniformLeafOrigin, voxelU);
      return VdbSampler_sample_inner(
          sampler, voxelU, domainOffset, time, attributeIndex);
    } else {
      // Varying traversal.
      uint64 voxel = vklVdbVoxelMakeEmpty();
      VdbSampler_dispatchInner_uniform_varying(sampler, 0ul, leafOrigin, voxel);
      return VdbSampler_sample_inner(
          sampler, voxel, domainOffset, time, attributeIndex);
    }
  }
  return 0.f;
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

  __vkl_leaf_handler(return VdbSampler_computeValueRange,
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

// ---------------------------------------------------------------------------
// NEAREST interpolation.
// ---------------------------------------------------------------------------

/*
 * Nearest neighbor interpolation is the fastest version, but also gives
 * blocky results. This should be good for indirect light etc.
 */
inline float VdbSampler_interpolateNearest(const VdbSampler *uniform sampler,
                                           const vec3f &indexCoordinates,
                                           const float &time,
                                           const uniform uint32 attributeIndex)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  return VdbSampler_traverseAndSample(sampler, ic, time, attributeIndex);
}

inline void VdbSampler_interpolateNearest(const VdbSampler *uniform sampler,
                                          const vec3f &indexCoordinates,
                                          const float &time,
                                          const uniform uint32 M,
                                          const uint32 *uniform
                                              attributeIndices,
                                          float *uniform samples)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uint64 voxel;
  vec3ui domainOffset;
  VdbSampler_traverse(sampler, ic, voxel, domainOffset);

  for (uniform unsigned int a = 0; a < M; a++) {
    const float samplesA = VdbSampler_sample(
        sampler, voxel, domainOffset, time, attributeIndices[a]);

    samples[a * VKL_TARGET_WIDTH + programIndex] = samplesA;
  }
}

inline uniform float VdbSampler_interpolateNearest(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform float time,
    const uniform uint32 attributeIndex)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  return VdbSampler_traverseAndSample(sampler, ic, time, attributeIndex);
}

inline void VdbSampler_interpolateNearest(const VdbSampler *uniform sampler,
                                          const uniform vec3f &indexCoordinates,
                                          const uniform float time,
                                          const uniform uint32 M,
                                          const uint32 *uniform
                                              attributeIndices,
                                          float *uniform samples)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform uint64 voxel;
  uniform vec3ui domainOffset;
  VdbSampler_traverse(sampler, ic, voxel, domainOffset);

  for (uniform unsigned int a = 0; a < M; a++) {
    samples[a] = VdbSampler_sample(
        sampler, voxel, domainOffset, time, attributeIndices[a]);
  }
}

inline void VdbSampler_interpolateNearest(const VdbSampler *uniform sampler,
                                          const uniform unsigned int N,
                                          const vec3f *uniform
                                              objectCoordinates,
                                          const float *uniform times,
                                          const uint32 uniform attributeIndex,
                                          float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const vec3i ic               = make_vec3i(floor(indexCoordinates.x),
                                              floor(indexCoordinates.y),
                                              floor(indexCoordinates.z));
    const float time             = times ? times[i] : 0.f;
    const float sample =
        VdbSampler_traverseAndSample(sampler, ic, time, attributeIndex);
    samples[i] = sample;
  }
}

inline void VdbSampler_interpolateNearest(
    const VdbSampler *uniform sampler,
    const uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    const vec3i ic   = make_vec3i(floor(indexCoordinates.x),
                                  floor(indexCoordinates.y),
                                  floor(indexCoordinates.z));
    const float time = times ? times[i] : 0.f;

    uint64 voxel;
    vec3ui domainOffset;
    VdbSampler_traverse(sampler, ic, voxel, domainOffset);

    for (uniform unsigned int a = 0; a < M; a++) {
      const float samplesA = VdbSampler_sample(
          sampler, voxel, domainOffset, time, attributeIndices[a]);

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
    const float &time,
    const uniform uint32 attributeIndex)
{
  return make_vec3f(0.f);
}

inline void VdbSampler_computeGradientNearest(
    const VdbSampler *uniform sampler,
    uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
    const uniform uint32 attributeIndex,
    vec3f *uniform gradients)
{
  foreach (i = 0 ... N) {
    gradients[i] = make_vec3f(0.f);
  }
}

// ---------------------------------------------------------------------------
// TRILINEAR interpolation.
// ---------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Filter stencils.
// -----------------------------------------------------------------------------

// Defines required for __vkl_stencil_dispatch
#define VKL_STENCIL_TRILINEAR_SIZE 8
#define VKL_STENCIL_TRILINEAR_OFFSETS                                 \
  {                                                                   \
    {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {0, 1, 1}, {1, 0, 0}, {1, 0, 1}, \
        {1, 1, 0},                                                    \
    {                                                                 \
      1, 1, 1                                                         \
    }                                                                 \
  }

/*
 * Compute voxel values for the eight corners required in trilinear
 * interpolation.
 * This is used for both sampling and gradient computation!
 */
inline void VdbSampler_computeVoxelValuesTrilinear(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
    const float &time,
    const uniform uint32 attributeIndex,
    float *uniform sample)  // Array of VKL_TARGET_WIDTH * 8 elements!
{
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, time, {
    sample[tgtIdx] =
        VdbSampler_traverseAndSample(sampler, icDisp, timeDisp, attributeIndex);
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
  const float time = 0.f;
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

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
    const float &time,
    const uniform uint32 attributeIndex,
    float *uniform sample)  // Array of VKL_TARGET_WIDTH * 8 elements!
{
  // Note: We might be able to avoid this dummy variable, but that would
  // complicate the dispatch macro.
  const vec3i ic = {0, 0, 0};
  __vkl_stencil_dispatch_varying(TRILINEAR, ic, time, {
    const uint64 voxelV        = voxel[tgtIdx];
    const vec3ui domainOffsetV = domainOffset[tgtIdx];
    sample[tgtIdx]             = VdbSampler_sample(
                    sampler, voxelV, domainOffsetV, timeDisp, attributeIndex);
  });
}

/*
 * Trilinear sampling is a good default for directly visible volumes.
 * The implementation is optimized to exploit SIMD.
 */
inline varying float VdbSampler_interpolateTrilinear(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const float &time,
    const uniform uint32 attributeIndex)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                 floor(indexCoordinates.y),
                                 floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(
      sampler, ic, time, attributeIndex, sample);

  const varying float *uniform s = (const varying float *uniform) & sample;
  return lerp(
      delta.x,
      lerp(delta.y, lerp(delta.z, s[0], s[1]), lerp(delta.z, s[2], s[3])),
      lerp(delta.y, lerp(delta.z, s[4], s[5]), lerp(delta.z, s[6], s[7])));
}

inline void VdbSampler_interpolateTrilinear(const VdbSampler *uniform sampler,
                                            const vec3f &indexCoordinates,
                                            const float &time,
                                            const uniform uint32 M,
                                            const uint32 *uniform
                                                attributeIndices,
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
        sampler, voxel, domainOffset, time, attributeIndices[a], sample);

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
inline uniform float VdbSampler_interpolateTrilinear(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform float time,
    const uniform uint32 attributeIndex)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform float sample[8];

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
    sample[tgtIdx] =
        VdbSampler_traverseAndSample(sampler, icDisp, timeDisp, attributeIndex);
  });

  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);
  return lerp(delta.x,
              lerp(delta.y,
                   lerp(delta.z, sample[0], sample[1]),
                   lerp(delta.z, sample[2], sample[3])),
              lerp(delta.y,
                   lerp(delta.z, sample[4], sample[5]),
                   lerp(delta.z, sample[6], sample[7])));
}

inline void VdbSampler_interpolateTrilinear(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform float time,
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

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  });

  for (uniform unsigned int a = 0; a < M; a++) {
    uniform float sample[8];

    __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
      const uint64 voxelV        = voxel[tgtIdx];
      const vec3ui domainOffsetV = domainOffset[tgtIdx];
      sample[tgtIdx]             = VdbSampler_sample(
                      sampler, voxelV, domainOffsetV, timeDisp, attributeIndices[a]);
    });

    samples[a] = lerp(delta.x,
                      lerp(delta.y,
                           lerp(delta.z, sample[0], sample[1]),
                           lerp(delta.z, sample[2], sample[3])),
                      lerp(delta.y,
                           lerp(delta.z, sample[4], sample[5]),
                           lerp(delta.z, sample[6], sample[7])));
  }
}

inline void VdbSampler_interpolateTrilinear(const VdbSampler *uniform sampler,
                                            const uniform unsigned int N,
                                            const vec3f *uniform
                                                objectCoordinates,
                                            const float *uniform times,
                                            const uint32 uniform attributeIndex,
                                            float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const float time             = times ? times[i] : 0.f;
    samples[i]                   = VdbSampler_interpolateTrilinear(
                          sampler, indexCoordinates, time, attributeIndex);
  }
}

inline void VdbSampler_interpolateTrilinear(
    const VdbSampler *uniform sampler,
    const uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
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
    const float time  = times ? times[i] : 0.f;

    uniform uint64 voxel[VKL_TARGET_WIDTH * 8];
    uniform vec3ui domainOffset[VKL_TARGET_WIDTH * 8];
    VdbSampler_traverseVoxelValuesTrilinear(sampler, ic, voxel, domainOffset);

    for (uniform unsigned int a = 0; a < M; a++) {
      uniform float sample[VKL_TARGET_WIDTH * 8];
      VdbSampler_computeVoxelValuesTrilinear(
          sampler, voxel, domainOffset, time, attributeIndices[a], sample);

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
 * Gradients in trilinear fields.
 */
inline vec3f VdbSampler_computeGradientTrilinear(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const float &time,
    const uniform uint32 attributeIndex)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                                 floor(indexCoordinates.y),
                                 floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(
      sampler, ic, time, attributeIndex, sample);

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

inline void VdbSampler_computeGradientTrilinear(
    const VdbSampler *uniform sampler,
    uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
    const uniform uint32 attributeIndex,
    vec3f *uniform gradients)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const float time             = times ? times[i] : 0.f;
    const vec3f gradient         = VdbSampler_computeGradientTrilinear(
                sampler, indexCoordinates, time, attributeIndex);
    // Note: xfmNormal takes inverse!
    gradients[i] = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

// ---------------------------------------------------------------------------
// TRICUBIC interpolation.
//
// We implement the technique from
//
// "Tricubic Interpolation in Three Dimensions"
// F. Lekien and J. Marsden
// Journal of Numerical Methods and Engineering (2005)
//
// Note that we use central differences to obtain partial derivatives for our
// constraints.
// ---------------------------------------------------------------------------

// Compute the coefficient vector COE (64 elements) from the constraint
// vector CTR (64 elements).
// CTR = [
//   f(0, 0, 0), f(0, 0, 1), ..., f(1, 1, 1),
//   d/dx f, ...,
//   d/dy f, ...,
//   d/dz f, ...,
//   d2/dxdy f, ...,
//   d2/dxdz f, ...,
//   d2/dydz f, ...,
//   d3/dxdydz f, ...
// ]
#define __vkl_tricubic_coefficients(CTR)                                       \
  {                                                                            \
    (1.0) * CTR[0], (1.0) * CTR[24],                                           \
        (-3.0) * CTR[0] + (3.0) * CTR[1] + (-2.0) * CTR[24] +                  \
            (-1.0) * CTR[25],                                                  \
        (2.0) * CTR[0] + (-2.0) * CTR[1] + (1.0) * CTR[24] + (1.0) * CTR[25],  \
        (1.0) * CTR[16], (1.0) * CTR[48],                                      \
        (-3.0) * CTR[16] + (3.0) * CTR[17] + (-2.0) * CTR[48] +                \
            (-1.0) * CTR[49],                                                  \
        (2.0) * CTR[16] + (-2.0) * CTR[17] + (1.0) * CTR[48] +                 \
            (1.0) * CTR[49],                                                   \
        (-3.0) * CTR[0] + (3.0) * CTR[2] + (-2.0) * CTR[16] +                  \
            (-1.0) * CTR[18],                                                  \
        (-3.0) * CTR[24] + (3.0) * CTR[26] + (-2.0) * CTR[48] +                \
            (-1.0) * CTR[50],                                                  \
        (9.0) * CTR[0] + (-9.0) * CTR[1] + (-9.0) * CTR[2] + (9.0) * CTR[3] +  \
            (6.0) * CTR[16] + (-6.0) * CTR[17] + (3.0) * CTR[18] +             \
            (-3.0) * CTR[19] + (6.0) * CTR[24] + (3.0) * CTR[25] +             \
            (-6.0) * CTR[26] + (-3.0) * CTR[27] + (4.0) * CTR[48] +            \
            (2.0) * CTR[49] + (2.0) * CTR[50] + (1.0) * CTR[51],               \
        (-6.0) * CTR[0] + (6.0) * CTR[1] + (6.0) * CTR[2] + (-6.0) * CTR[3] +  \
            (-4.0) * CTR[16] + (4.0) * CTR[17] + (-2.0) * CTR[18] +            \
            (2.0) * CTR[19] + (-3.0) * CTR[24] + (-3.0) * CTR[25] +            \
            (3.0) * CTR[26] + (3.0) * CTR[27] + (-2.0) * CTR[48] +             \
            (-2.0) * CTR[49] + (-1.0) * CTR[50] + (-1.0) * CTR[51],            \
        (2.0) * CTR[0] + (-2.0) * CTR[2] + (1.0) * CTR[16] + (1.0) * CTR[18],  \
        (2.0) * CTR[24] + (-2.0) * CTR[26] + (1.0) * CTR[48] +                 \
            (1.0) * CTR[50],                                                   \
        (-6.0) * CTR[0] + (6.0) * CTR[1] + (6.0) * CTR[2] + (-6.0) * CTR[3] +  \
            (-3.0) * CTR[16] + (3.0) * CTR[17] + (-3.0) * CTR[18] +            \
            (3.0) * CTR[19] + (-4.0) * CTR[24] + (-2.0) * CTR[25] +            \
            (4.0) * CTR[26] + (2.0) * CTR[27] + (-2.0) * CTR[48] +             \
            (-1.0) * CTR[49] + (-2.0) * CTR[50] + (-1.0) * CTR[51],            \
        (4.0) * CTR[0] + (-4.0) * CTR[1] + (-4.0) * CTR[2] + (4.0) * CTR[3] +  \
            (2.0) * CTR[16] + (-2.0) * CTR[17] + (2.0) * CTR[18] +             \
            (-2.0) * CTR[19] + (2.0) * CTR[24] + (2.0) * CTR[25] +             \
            (-2.0) * CTR[26] + (-2.0) * CTR[27] + (1.0) * CTR[48] +            \
            (1.0) * CTR[49] + (1.0) * CTR[50] + (1.0) * CTR[51],               \
        (1.0) * CTR[8], (1.0) * CTR[40],                                       \
        (-3.0) * CTR[8] + (3.0) * CTR[9] + (-2.0) * CTR[40] +                  \
            (-1.0) * CTR[41],                                                  \
        (2.0) * CTR[8] + (-2.0) * CTR[9] + (1.0) * CTR[40] + (1.0) * CTR[41],  \
        (1.0) * CTR[32], (1.0) * CTR[56],                                      \
        (-3.0) * CTR[32] + (3.0) * CTR[33] + (-2.0) * CTR[56] +                \
            (-1.0) * CTR[57],                                                  \
        (2.0) * CTR[32] + (-2.0) * CTR[33] + (1.0) * CTR[56] +                 \
            (1.0) * CTR[57],                                                   \
        (-3.0) * CTR[8] + (3.0) * CTR[10] + (-2.0) * CTR[32] +                 \
            (-1.0) * CTR[34],                                                  \
        (-3.0) * CTR[40] + (3.0) * CTR[42] + (-2.0) * CTR[56] +                \
            (-1.0) * CTR[58],                                                  \
        (9.0) * CTR[8] + (-9.0) * CTR[9] + (-9.0) * CTR[10] +                  \
            (9.0) * CTR[11] + (6.0) * CTR[32] + (-6.0) * CTR[33] +             \
            (3.0) * CTR[34] + (-3.0) * CTR[35] + (6.0) * CTR[40] +             \
            (3.0) * CTR[41] + (-6.0) * CTR[42] + (-3.0) * CTR[43] +            \
            (4.0) * CTR[56] + (2.0) * CTR[57] + (2.0) * CTR[58] +              \
            (1.0) * CTR[59],                                                   \
        (-6.0) * CTR[8] + (6.0) * CTR[9] + (6.0) * CTR[10] +                   \
            (-6.0) * CTR[11] + (-4.0) * CTR[32] + (4.0) * CTR[33] +            \
            (-2.0) * CTR[34] + (2.0) * CTR[35] + (-3.0) * CTR[40] +            \
            (-3.0) * CTR[41] + (3.0) * CTR[42] + (3.0) * CTR[43] +             \
            (-2.0) * CTR[56] + (-2.0) * CTR[57] + (-1.0) * CTR[58] +           \
            (-1.0) * CTR[59],                                                  \
        (2.0) * CTR[8] + (-2.0) * CTR[10] + (1.0) * CTR[32] + (1.0) * CTR[34], \
        (2.0) * CTR[40] + (-2.0) * CTR[42] + (1.0) * CTR[56] +                 \
            (1.0) * CTR[58],                                                   \
        (-6.0) * CTR[8] + (6.0) * CTR[9] + (6.0) * CTR[10] +                   \
            (-6.0) * CTR[11] + (-3.0) * CTR[32] + (3.0) * CTR[33] +            \
            (-3.0) * CTR[34] + (3.0) * CTR[35] + (-4.0) * CTR[40] +            \
            (-2.0) * CTR[41] + (4.0) * CTR[42] + (2.0) * CTR[43] +             \
            (-2.0) * CTR[56] + (-1.0) * CTR[57] + (-2.0) * CTR[58] +           \
            (-1.0) * CTR[59],                                                  \
        (4.0) * CTR[8] + (-4.0) * CTR[9] + (-4.0) * CTR[10] +                  \
            (4.0) * CTR[11] + (2.0) * CTR[32] + (-2.0) * CTR[33] +             \
            (2.0) * CTR[34] + (-2.0) * CTR[35] + (2.0) * CTR[40] +             \
            (2.0) * CTR[41] + (-2.0) * CTR[42] + (-2.0) * CTR[43] +            \
            (1.0) * CTR[56] + (1.0) * CTR[57] + (1.0) * CTR[58] +              \
            (1.0) * CTR[59],                                                   \
        (-3.0) * CTR[0] + (3.0) * CTR[4] + (-2.0) * CTR[8] + (-1.0) * CTR[12], \
        (-3.0) * CTR[24] + (3.0) * CTR[28] + (-2.0) * CTR[40] +                \
            (-1.0) * CTR[44],                                                  \
        (9.0) * CTR[0] + (-9.0) * CTR[1] + (-9.0) * CTR[4] + (9.0) * CTR[5] +  \
            (6.0) * CTR[8] + (-6.0) * CTR[9] + (3.0) * CTR[12] +               \
            (-3.0) * CTR[13] + (6.0) * CTR[24] + (3.0) * CTR[25] +             \
            (-6.0) * CTR[28] + (-3.0) * CTR[29] + (4.0) * CTR[40] +            \
            (2.0) * CTR[41] + (2.0) * CTR[44] + (1.0) * CTR[45],               \
        (-6.0) * CTR[0] + (6.0) * CTR[1] + (6.0) * CTR[4] + (-6.0) * CTR[5] +  \
            (-4.0) * CTR[8] + (4.0) * CTR[9] + (-2.0) * CTR[12] +              \
            (2.0) * CTR[13] + (-3.0) * CTR[24] + (-3.0) * CTR[25] +            \
            (3.0) * CTR[28] + (3.0) * CTR[29] + (-2.0) * CTR[40] +             \
            (-2.0) * CTR[41] + (-1.0) * CTR[44] + (-1.0) * CTR[45],            \
        (-3.0) * CTR[16] + (3.0) * CTR[20] + (-2.0) * CTR[32] +                \
            (-1.0) * CTR[36],                                                  \
        (-3.0) * CTR[48] + (3.0) * CTR[52] + (-2.0) * CTR[56] +                \
            (-1.0) * CTR[60],                                                  \
        (9.0) * CTR[16] + (-9.0) * CTR[17] + (-9.0) * CTR[20] +                \
            (9.0) * CTR[21] + (6.0) * CTR[32] + (-6.0) * CTR[33] +             \
            (3.0) * CTR[36] + (-3.0) * CTR[37] + (6.0) * CTR[48] +             \
            (3.0) * CTR[49] + (-6.0) * CTR[52] + (-3.0) * CTR[53] +            \
            (4.0) * CTR[56] + (2.0) * CTR[57] + (2.0) * CTR[60] +              \
            (1.0) * CTR[61],                                                   \
        (-6.0) * CTR[16] + (6.0) * CTR[17] + (6.0) * CTR[20] +                 \
            (-6.0) * CTR[21] + (-4.0) * CTR[32] + (4.0) * CTR[33] +            \
            (-2.0) * CTR[36] + (2.0) * CTR[37] + (-3.0) * CTR[48] +            \
            (-3.0) * CTR[49] + (3.0) * CTR[52] + (3.0) * CTR[53] +             \
            (-2.0) * CTR[56] + (-2.0) * CTR[57] + (-1.0) * CTR[60] +           \
            (-1.0) * CTR[61],                                                  \
        (9.0) * CTR[0] + (-9.0) * CTR[2] + (-9.0) * CTR[4] + (9.0) * CTR[6] +  \
            (6.0) * CTR[8] + (-6.0) * CTR[10] + (3.0) * CTR[12] +              \
            (-3.0) * CTR[14] + (6.0) * CTR[16] + (3.0) * CTR[18] +             \
            (-6.0) * CTR[20] + (-3.0) * CTR[22] + (4.0) * CTR[32] +            \
            (2.0) * CTR[34] + (2.0) * CTR[36] + (1.0) * CTR[38],               \
        (9.0) * CTR[24] + (-9.0) * CTR[26] + (-9.0) * CTR[28] +                \
            (9.0) * CTR[30] + (6.0) * CTR[40] + (-6.0) * CTR[42] +             \
            (3.0) * CTR[44] + (-3.0) * CTR[46] + (6.0) * CTR[48] +             \
            (3.0) * CTR[50] + (-6.0) * CTR[52] + (-3.0) * CTR[54] +            \
            (4.0) * CTR[56] + (2.0) * CTR[58] + (2.0) * CTR[60] +              \
            (1.0) * CTR[62],                                                   \
        (-27.0) * CTR[0] + (27.0) * CTR[1] + (27.0) * CTR[2] +                 \
            (-27.0) * CTR[3] + (27.0) * CTR[4] + (-27.0) * CTR[5] +            \
            (-27.0) * CTR[6] + (27.0) * CTR[7] + (-18.0) * CTR[8] +            \
            (18.0) * CTR[9] + (18.0) * CTR[10] + (-18.0) * CTR[11] +           \
            (-9.0) * CTR[12] + (9.0) * CTR[13] + (9.0) * CTR[14] +             \
            (-9.0) * CTR[15] + (-18.0) * CTR[16] + (18.0) * CTR[17] +          \
            (-9.0) * CTR[18] + (9.0) * CTR[19] + (18.0) * CTR[20] +            \
            (-18.0) * CTR[21] + (9.0) * CTR[22] + (-9.0) * CTR[23] +           \
            (-18.0) * CTR[24] + (-9.0) * CTR[25] + (18.0) * CTR[26] +          \
            (9.0) * CTR[27] + (18.0) * CTR[28] + (9.0) * CTR[29] +             \
            (-18.0) * CTR[30] + (-9.0) * CTR[31] + (-12.0) * CTR[32] +         \
            (12.0) * CTR[33] + (-6.0) * CTR[34] + (6.0) * CTR[35] +            \
            (-6.0) * CTR[36] + (6.0) * CTR[37] + (-3.0) * CTR[38] +            \
            (3.0) * CTR[39] + (-12.0) * CTR[40] + (-6.0) * CTR[41] +           \
            (12.0) * CTR[42] + (6.0) * CTR[43] + (-6.0) * CTR[44] +            \
            (-3.0) * CTR[45] + (6.0) * CTR[46] + (3.0) * CTR[47] +             \
            (-12.0) * CTR[48] + (-6.0) * CTR[49] + (-6.0) * CTR[50] +          \
            (-3.0) * CTR[51] + (12.0) * CTR[52] + (6.0) * CTR[53] +            \
            (6.0) * CTR[54] + (3.0) * CTR[55] + (-8.0) * CTR[56] +             \
            (-4.0) * CTR[57] + (-4.0) * CTR[58] + (-2.0) * CTR[59] +           \
            (-4.0) * CTR[60] + (-2.0) * CTR[61] + (-2.0) * CTR[62] +           \
            (-1.0) * CTR[63],                                                  \
        (18.0) * CTR[0] + (-18.0) * CTR[1] + (-18.0) * CTR[2] +                \
            (18.0) * CTR[3] + (-18.0) * CTR[4] + (18.0) * CTR[5] +             \
            (18.0) * CTR[6] + (-18.0) * CTR[7] + (12.0) * CTR[8] +             \
            (-12.0) * CTR[9] + (-12.0) * CTR[10] + (12.0) * CTR[11] +          \
            (6.0) * CTR[12] + (-6.0) * CTR[13] + (-6.0) * CTR[14] +            \
            (6.0) * CTR[15] + (12.0) * CTR[16] + (-12.0) * CTR[17] +           \
            (6.0) * CTR[18] + (-6.0) * CTR[19] + (-12.0) * CTR[20] +           \
            (12.0) * CTR[21] + (-6.0) * CTR[22] + (6.0) * CTR[23] +            \
            (9.0) * CTR[24] + (9.0) * CTR[25] + (-9.0) * CTR[26] +             \
            (-9.0) * CTR[27] + (-9.0) * CTR[28] + (-9.0) * CTR[29] +           \
            (9.0) * CTR[30] + (9.0) * CTR[31] + (8.0) * CTR[32] +              \
            (-8.0) * CTR[33] + (4.0) * CTR[34] + (-4.0) * CTR[35] +            \
            (4.0) * CTR[36] + (-4.0) * CTR[37] + (2.0) * CTR[38] +             \
            (-2.0) * CTR[39] + (6.0) * CTR[40] + (6.0) * CTR[41] +             \
            (-6.0) * CTR[42] + (-6.0) * CTR[43] + (3.0) * CTR[44] +            \
            (3.0) * CTR[45] + (-3.0) * CTR[46] + (-3.0) * CTR[47] +            \
            (6.0) * CTR[48] + (6.0) * CTR[49] + (3.0) * CTR[50] +              \
            (3.0) * CTR[51] + (-6.0) * CTR[52] + (-6.0) * CTR[53] +            \
            (-3.0) * CTR[54] + (-3.0) * CTR[55] + (4.0) * CTR[56] +            \
            (4.0) * CTR[57] + (2.0) * CTR[58] + (2.0) * CTR[59] +              \
            (2.0) * CTR[60] + (2.0) * CTR[61] + (1.0) * CTR[62] +              \
            (1.0) * CTR[63],                                                   \
        (-6.0) * CTR[0] + (6.0) * CTR[2] + (6.0) * CTR[4] + (-6.0) * CTR[6] +  \
            (-4.0) * CTR[8] + (4.0) * CTR[10] + (-2.0) * CTR[12] +             \
            (2.0) * CTR[14] + (-3.0) * CTR[16] + (-3.0) * CTR[18] +            \
            (3.0) * CTR[20] + (3.0) * CTR[22] + (-2.0) * CTR[32] +             \
            (-2.0) * CTR[34] + (-1.0) * CTR[36] + (-1.0) * CTR[38],            \
        (-6.0) * CTR[24] + (6.0) * CTR[26] + (6.0) * CTR[28] +                 \
            (-6.0) * CTR[30] + (-4.0) * CTR[40] + (4.0) * CTR[42] +            \
            (-2.0) * CTR[44] + (2.0) * CTR[46] + (-3.0) * CTR[48] +            \
            (-3.0) * CTR[50] + (3.0) * CTR[52] + (3.0) * CTR[54] +             \
            (-2.0) * CTR[56] + (-2.0) * CTR[58] + (-1.0) * CTR[60] +           \
            (-1.0) * CTR[62],                                                  \
        (18.0) * CTR[0] + (-18.0) * CTR[1] + (-18.0) * CTR[2] +                \
            (18.0) * CTR[3] + (-18.0) * CTR[4] + (18.0) * CTR[5] +             \
            (18.0) * CTR[6] + (-18.0) * CTR[7] + (12.0) * CTR[8] +             \
            (-12.0) * CTR[9] + (-12.0) * CTR[10] + (12.0) * CTR[11] +          \
            (6.0) * CTR[12] + (-6.0) * CTR[13] + (-6.0) * CTR[14] +            \
            (6.0) * CTR[15] + (9.0) * CTR[16] + (-9.0) * CTR[17] +             \
            (9.0) * CTR[18] + (-9.0) * CTR[19] + (-9.0) * CTR[20] +            \
            (9.0) * CTR[21] + (-9.0) * CTR[22] + (9.0) * CTR[23] +             \
            (12.0) * CTR[24] + (6.0) * CTR[25] + (-12.0) * CTR[26] +           \
            (-6.0) * CTR[27] + (-12.0) * CTR[28] + (-6.0) * CTR[29] +          \
            (12.0) * CTR[30] + (6.0) * CTR[31] + (6.0) * CTR[32] +             \
            (-6.0) * CTR[33] + (6.0) * CTR[34] + (-6.0) * CTR[35] +            \
            (3.0) * CTR[36] + (-3.0) * CTR[37] + (3.0) * CTR[38] +             \
            (-3.0) * CTR[39] + (8.0) * CTR[40] + (4.0) * CTR[41] +             \
            (-8.0) * CTR[42] + (-4.0) * CTR[43] + (4.0) * CTR[44] +            \
            (2.0) * CTR[45] + (-4.0) * CTR[46] + (-2.0) * CTR[47] +            \
            (6.0) * CTR[48] + (3.0) * CTR[49] + (6.0) * CTR[50] +              \
            (3.0) * CTR[51] + (-6.0) * CTR[52] + (-3.0) * CTR[53] +            \
            (-6.0) * CTR[54] + (-3.0) * CTR[55] + (4.0) * CTR[56] +            \
            (2.0) * CTR[57] + (4.0) * CTR[58] + (2.0) * CTR[59] +              \
            (2.0) * CTR[60] + (1.0) * CTR[61] + (2.0) * CTR[62] +              \
            (1.0) * CTR[63],                                                   \
        (-12.0) * CTR[0] + (12.0) * CTR[1] + (12.0) * CTR[2] +                 \
            (-12.0) * CTR[3] + (12.0) * CTR[4] + (-12.0) * CTR[5] +            \
            (-12.0) * CTR[6] + (12.0) * CTR[7] + (-8.0) * CTR[8] +             \
            (8.0) * CTR[9] + (8.0) * CTR[10] + (-8.0) * CTR[11] +              \
            (-4.0) * CTR[12] + (4.0) * CTR[13] + (4.0) * CTR[14] +             \
            (-4.0) * CTR[15] + (-6.0) * CTR[16] + (6.0) * CTR[17] +            \
            (-6.0) * CTR[18] + (6.0) * CTR[19] + (6.0) * CTR[20] +             \
            (-6.0) * CTR[21] + (6.0) * CTR[22] + (-6.0) * CTR[23] +            \
            (-6.0) * CTR[24] + (-6.0) * CTR[25] + (6.0) * CTR[26] +            \
            (6.0) * CTR[27] + (6.0) * CTR[28] + (6.0) * CTR[29] +              \
            (-6.0) * CTR[30] + (-6.0) * CTR[31] + (-4.0) * CTR[32] +           \
            (4.0) * CTR[33] + (-4.0) * CTR[34] + (4.0) * CTR[35] +             \
            (-2.0) * CTR[36] + (2.0) * CTR[37] + (-2.0) * CTR[38] +            \
            (2.0) * CTR[39] + (-4.0) * CTR[40] + (-4.0) * CTR[41] +            \
            (4.0) * CTR[42] + (4.0) * CTR[43] + (-2.0) * CTR[44] +             \
            (-2.0) * CTR[45] + (2.0) * CTR[46] + (2.0) * CTR[47] +             \
            (-3.0) * CTR[48] + (-3.0) * CTR[49] + (-3.0) * CTR[50] +           \
            (-3.0) * CTR[51] + (3.0) * CTR[52] + (3.0) * CTR[53] +             \
            (3.0) * CTR[54] + (3.0) * CTR[55] + (-2.0) * CTR[56] +             \
            (-2.0) * CTR[57] + (-2.0) * CTR[58] + (-2.0) * CTR[59] +           \
            (-1.0) * CTR[60] + (-1.0) * CTR[61] + (-1.0) * CTR[62] +           \
            (-1.0) * CTR[63],                                                  \
        (2.0) * CTR[0] + (-2.0) * CTR[4] + (1.0) * CTR[8] + (1.0) * CTR[12],   \
        (2.0) * CTR[24] + (-2.0) * CTR[28] + (1.0) * CTR[40] +                 \
            (1.0) * CTR[44],                                                   \
        (-6.0) * CTR[0] + (6.0) * CTR[1] + (6.0) * CTR[4] + (-6.0) * CTR[5] +  \
            (-3.0) * CTR[8] + (3.0) * CTR[9] + (-3.0) * CTR[12] +              \
            (3.0) * CTR[13] + (-4.0) * CTR[24] + (-2.0) * CTR[25] +            \
            (4.0) * CTR[28] + (2.0) * CTR[29] + (-2.0) * CTR[40] +             \
            (-1.0) * CTR[41] + (-2.0) * CTR[44] + (-1.0) * CTR[45],            \
        (4.0) * CTR[0] + (-4.0) * CTR[1] + (-4.0) * CTR[4] + (4.0) * CTR[5] +  \
            (2.0) * CTR[8] + (-2.0) * CTR[9] + (2.0) * CTR[12] +               \
            (-2.0) * CTR[13] + (2.0) * CTR[24] + (2.0) * CTR[25] +             \
            (-2.0) * CTR[28] + (-2.0) * CTR[29] + (1.0) * CTR[40] +            \
            (1.0) * CTR[41] + (1.0) * CTR[44] + (1.0) * CTR[45],               \
        (2.0) * CTR[16] + (-2.0) * CTR[20] + (1.0) * CTR[32] +                 \
            (1.0) * CTR[36],                                                   \
        (2.0) * CTR[48] + (-2.0) * CTR[52] + (1.0) * CTR[56] +                 \
            (1.0) * CTR[60],                                                   \
        (-6.0) * CTR[16] + (6.0) * CTR[17] + (6.0) * CTR[20] +                 \
            (-6.0) * CTR[21] + (-3.0) * CTR[32] + (3.0) * CTR[33] +            \
            (-3.0) * CTR[36] + (3.0) * CTR[37] + (-4.0) * CTR[48] +            \
            (-2.0) * CTR[49] + (4.0) * CTR[52] + (2.0) * CTR[53] +             \
            (-2.0) * CTR[56] + (-1.0) * CTR[57] + (-2.0) * CTR[60] +           \
            (-1.0) * CTR[61],                                                  \
        (4.0) * CTR[16] + (-4.0) * CTR[17] + (-4.0) * CTR[20] +                \
            (4.0) * CTR[21] + (2.0) * CTR[32] + (-2.0) * CTR[33] +             \
            (2.0) * CTR[36] + (-2.0) * CTR[37] + (2.0) * CTR[48] +             \
            (2.0) * CTR[49] + (-2.0) * CTR[52] + (-2.0) * CTR[53] +            \
            (1.0) * CTR[56] + (1.0) * CTR[57] + (1.0) * CTR[60] +              \
            (1.0) * CTR[61],                                                   \
        (-6.0) * CTR[0] + (6.0) * CTR[2] + (6.0) * CTR[4] + (-6.0) * CTR[6] +  \
            (-3.0) * CTR[8] + (3.0) * CTR[10] + (-3.0) * CTR[12] +             \
            (3.0) * CTR[14] + (-4.0) * CTR[16] + (-2.0) * CTR[18] +            \
            (4.0) * CTR[20] + (2.0) * CTR[22] + (-2.0) * CTR[32] +             \
            (-1.0) * CTR[34] + (-2.0) * CTR[36] + (-1.0) * CTR[38],            \
        (-6.0) * CTR[24] + (6.0) * CTR[26] + (6.0) * CTR[28] +                 \
            (-6.0) * CTR[30] + (-3.0) * CTR[40] + (3.0) * CTR[42] +            \
            (-3.0) * CTR[44] + (3.0) * CTR[46] + (-4.0) * CTR[48] +            \
            (-2.0) * CTR[50] + (4.0) * CTR[52] + (2.0) * CTR[54] +             \
            (-2.0) * CTR[56] + (-1.0) * CTR[58] + (-2.0) * CTR[60] +           \
            (-1.0) * CTR[62],                                                  \
        (18.0) * CTR[0] + (-18.0) * CTR[1] + (-18.0) * CTR[2] +                \
            (18.0) * CTR[3] + (-18.0) * CTR[4] + (18.0) * CTR[5] +             \
            (18.0) * CTR[6] + (-18.0) * CTR[7] + (9.0) * CTR[8] +              \
            (-9.0) * CTR[9] + (-9.0) * CTR[10] + (9.0) * CTR[11] +             \
            (9.0) * CTR[12] + (-9.0) * CTR[13] + (-9.0) * CTR[14] +            \
            (9.0) * CTR[15] + (12.0) * CTR[16] + (-12.0) * CTR[17] +           \
            (6.0) * CTR[18] + (-6.0) * CTR[19] + (-12.0) * CTR[20] +           \
            (12.0) * CTR[21] + (-6.0) * CTR[22] + (6.0) * CTR[23] +            \
            (12.0) * CTR[24] + (6.0) * CTR[25] + (-12.0) * CTR[26] +           \
            (-6.0) * CTR[27] + (-12.0) * CTR[28] + (-6.0) * CTR[29] +          \
            (12.0) * CTR[30] + (6.0) * CTR[31] + (6.0) * CTR[32] +             \
            (-6.0) * CTR[33] + (3.0) * CTR[34] + (-3.0) * CTR[35] +            \
            (6.0) * CTR[36] + (-6.0) * CTR[37] + (3.0) * CTR[38] +             \
            (-3.0) * CTR[39] + (6.0) * CTR[40] + (3.0) * CTR[41] +             \
            (-6.0) * CTR[42] + (-3.0) * CTR[43] + (6.0) * CTR[44] +            \
            (3.0) * CTR[45] + (-6.0) * CTR[46] + (-3.0) * CTR[47] +            \
            (8.0) * CTR[48] + (4.0) * CTR[49] + (4.0) * CTR[50] +              \
            (2.0) * CTR[51] + (-8.0) * CTR[52] + (-4.0) * CTR[53] +            \
            (-4.0) * CTR[54] + (-2.0) * CTR[55] + (4.0) * CTR[56] +            \
            (2.0) * CTR[57] + (2.0) * CTR[58] + (1.0) * CTR[59] +              \
            (4.0) * CTR[60] + (2.0) * CTR[61] + (2.0) * CTR[62] +              \
            (1.0) * CTR[63],                                                   \
        (-12.0) * CTR[0] + (12.0) * CTR[1] + (12.0) * CTR[2] +                 \
            (-12.0) * CTR[3] + (12.0) * CTR[4] + (-12.0) * CTR[5] +            \
            (-12.0) * CTR[6] + (12.0) * CTR[7] + (-6.0) * CTR[8] +             \
            (6.0) * CTR[9] + (6.0) * CTR[10] + (-6.0) * CTR[11] +              \
            (-6.0) * CTR[12] + (6.0) * CTR[13] + (6.0) * CTR[14] +             \
            (-6.0) * CTR[15] + (-8.0) * CTR[16] + (8.0) * CTR[17] +            \
            (-4.0) * CTR[18] + (4.0) * CTR[19] + (8.0) * CTR[20] +             \
            (-8.0) * CTR[21] + (4.0) * CTR[22] + (-4.0) * CTR[23] +            \
            (-6.0) * CTR[24] + (-6.0) * CTR[25] + (6.0) * CTR[26] +            \
            (6.0) * CTR[27] + (6.0) * CTR[28] + (6.0) * CTR[29] +              \
            (-6.0) * CTR[30] + (-6.0) * CTR[31] + (-4.0) * CTR[32] +           \
            (4.0) * CTR[33] + (-2.0) * CTR[34] + (2.0) * CTR[35] +             \
            (-4.0) * CTR[36] + (4.0) * CTR[37] + (-2.0) * CTR[38] +            \
            (2.0) * CTR[39] + (-3.0) * CTR[40] + (-3.0) * CTR[41] +            \
            (3.0) * CTR[42] + (3.0) * CTR[43] + (-3.0) * CTR[44] +             \
            (-3.0) * CTR[45] + (3.0) * CTR[46] + (3.0) * CTR[47] +             \
            (-4.0) * CTR[48] + (-4.0) * CTR[49] + (-2.0) * CTR[50] +           \
            (-2.0) * CTR[51] + (4.0) * CTR[52] + (4.0) * CTR[53] +             \
            (2.0) * CTR[54] + (2.0) * CTR[55] + (-2.0) * CTR[56] +             \
            (-2.0) * CTR[57] + (-1.0) * CTR[58] + (-1.0) * CTR[59] +           \
            (-2.0) * CTR[60] + (-2.0) * CTR[61] + (-1.0) * CTR[62] +           \
            (-1.0) * CTR[63],                                                  \
        (4.0) * CTR[0] + (-4.0) * CTR[2] + (-4.0) * CTR[4] + (4.0) * CTR[6] +  \
            (2.0) * CTR[8] + (-2.0) * CTR[10] + (2.0) * CTR[12] +              \
            (-2.0) * CTR[14] + (2.0) * CTR[16] + (2.0) * CTR[18] +             \
            (-2.0) * CTR[20] + (-2.0) * CTR[22] + (1.0) * CTR[32] +            \
            (1.0) * CTR[34] + (1.0) * CTR[36] + (1.0) * CTR[38],               \
        (4.0) * CTR[24] + (-4.0) * CTR[26] + (-4.0) * CTR[28] +                \
            (4.0) * CTR[30] + (2.0) * CTR[40] + (-2.0) * CTR[42] +             \
            (2.0) * CTR[44] + (-2.0) * CTR[46] + (2.0) * CTR[48] +             \
            (2.0) * CTR[50] + (-2.0) * CTR[52] + (-2.0) * CTR[54] +            \
            (1.0) * CTR[56] + (1.0) * CTR[58] + (1.0) * CTR[60] +              \
            (1.0) * CTR[62],                                                   \
        (-12.0) * CTR[0] + (12.0) * CTR[1] + (12.0) * CTR[2] +                 \
            (-12.0) * CTR[3] + (12.0) * CTR[4] + (-12.0) * CTR[5] +            \
            (-12.0) * CTR[6] + (12.0) * CTR[7] + (-6.0) * CTR[8] +             \
            (6.0) * CTR[9] + (6.0) * CTR[10] + (-6.0) * CTR[11] +              \
            (-6.0) * CTR[12] + (6.0) * CTR[13] + (6.0) * CTR[14] +             \
            (-6.0) * CTR[15] + (-6.0) * CTR[16] + (6.0) * CTR[17] +            \
            (-6.0) * CTR[18] + (6.0) * CTR[19] + (6.0) * CTR[20] +             \
            (-6.0) * CTR[21] + (6.0) * CTR[22] + (-6.0) * CTR[23] +            \
            (-8.0) * CTR[24] + (-4.0) * CTR[25] + (8.0) * CTR[26] +            \
            (4.0) * CTR[27] + (8.0) * CTR[28] + (4.0) * CTR[29] +              \
            (-8.0) * CTR[30] + (-4.0) * CTR[31] + (-3.0) * CTR[32] +           \
            (3.0) * CTR[33] + (-3.0) * CTR[34] + (3.0) * CTR[35] +             \
            (-3.0) * CTR[36] + (3.0) * CTR[37] + (-3.0) * CTR[38] +            \
            (3.0) * CTR[39] + (-4.0) * CTR[40] + (-2.0) * CTR[41] +            \
            (4.0) * CTR[42] + (2.0) * CTR[43] + (-4.0) * CTR[44] +             \
            (-2.0) * CTR[45] + (4.0) * CTR[46] + (2.0) * CTR[47] +             \
            (-4.0) * CTR[48] + (-2.0) * CTR[49] + (-4.0) * CTR[50] +           \
            (-2.0) * CTR[51] + (4.0) * CTR[52] + (2.0) * CTR[53] +             \
            (4.0) * CTR[54] + (2.0) * CTR[55] + (-2.0) * CTR[56] +             \
            (-1.0) * CTR[57] + (-2.0) * CTR[58] + (-1.0) * CTR[59] +           \
            (-2.0) * CTR[60] + (-1.0) * CTR[61] + (-2.0) * CTR[62] +           \
            (-1.0) * CTR[63],                                                  \
        (8.0) * CTR[0] + (-8.0) * CTR[1] + (-8.0) * CTR[2] + (8.0) * CTR[3] +  \
            (-8.0) * CTR[4] + (8.0) * CTR[5] + (8.0) * CTR[6] +                \
            (-8.0) * CTR[7] + (4.0) * CTR[8] + (-4.0) * CTR[9] +               \
            (-4.0) * CTR[10] + (4.0) * CTR[11] + (4.0) * CTR[12] +             \
            (-4.0) * CTR[13] + (-4.0) * CTR[14] + (4.0) * CTR[15] +            \
            (4.0) * CTR[16] + (-4.0) * CTR[17] + (4.0) * CTR[18] +             \
            (-4.0) * CTR[19] + (-4.0) * CTR[20] + (4.0) * CTR[21] +            \
            (-4.0) * CTR[22] + (4.0) * CTR[23] + (4.0) * CTR[24] +             \
            (4.0) * CTR[25] + (-4.0) * CTR[26] + (-4.0) * CTR[27] +            \
            (-4.0) * CTR[28] + (-4.0) * CTR[29] + (4.0) * CTR[30] +            \
            (4.0) * CTR[31] + (2.0) * CTR[32] + (-2.0) * CTR[33] +             \
            (2.0) * CTR[34] + (-2.0) * CTR[35] + (2.0) * CTR[36] +             \
            (-2.0) * CTR[37] + (2.0) * CTR[38] + (-2.0) * CTR[39] +            \
            (2.0) * CTR[40] + (2.0) * CTR[41] + (-2.0) * CTR[42] +             \
            (-2.0) * CTR[43] + (2.0) * CTR[44] + (2.0) * CTR[45] +             \
            (-2.0) * CTR[46] + (-2.0) * CTR[47] + (2.0) * CTR[48] +            \
            (2.0) * CTR[49] + (2.0) * CTR[50] + (2.0) * CTR[51] +              \
            (-2.0) * CTR[52] + (-2.0) * CTR[53] + (-2.0) * CTR[54] +           \
            (-2.0) * CTR[55] + (1.0) * CTR[56] + (1.0) * CTR[57] +             \
            (1.0) * CTR[58] + (1.0) * CTR[59] + (1.0) * CTR[60] +              \
            (1.0) * CTR[61] + (1.0) * CTR[62] + (1.0) * CTR[63]                \
  }

#define VKL_STENCIL_TRICUBIC_RES 4
#define VKL_STENCIL_TRICUBIC_SIZE 64
#define VKL_STENCIL_TRICUBIC_OFFSETS                                          \
  {                                                                           \
    {-1, -1, -1}, {-1, -1, 0}, {-1, -1, 1}, {-1, -1, 2}, {-1, 0, -1},         \
        {-1, 0, 0}, {-1, 0, 1}, {-1, 0, 2}, {-1, 1, -1}, {-1, 1, 0},          \
        {-1, 1, 1}, {-1, 1, 2}, {-1, 2, -1}, {-1, 2, 0}, {-1, 2, 1},          \
        {-1, 2, 2}, {0, -1, -1}, {0, -1, 0}, {0, -1, 1}, {0, -1, 2},          \
        {0, 0, -1}, {0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 1, -1}, {0, 1, 0},   \
        {0, 1, 1}, {0, 1, 2}, {0, 2, -1}, {0, 2, 0}, {0, 2, 1}, {0, 2, 2},    \
        {1, -1, -1}, {1, -1, 0}, {1, -1, 1}, {1, -1, 2}, {1, 0, -1},          \
        {1, 0, 0}, {1, 0, 1}, {1, 0, 2}, {1, 1, -1}, {1, 1, 0}, {1, 1, 1},    \
        {1, 1, 2}, {1, 2, -1}, {1, 2, 0}, {1, 2, 1}, {1, 2, 2}, {2, -1, -1},  \
        {2, -1, 0}, {2, -1, 1}, {2, -1, 2}, {2, 0, -1}, {2, 0, 0}, {2, 0, 1}, \
        {2, 0, 2}, {2, 1, -1}, {2, 1, 0}, {2, 1, 1}, {2, 1, 2}, {2, 2, -1},   \
        {2, 2, 0}, {2, 2, 1}, {2, 2, 2},                                      \
  }

// In the macros below, x, y, z must be in [0, 1].

#define __vkl_tricubic_spl(SPL, x, y, z)                               \
  SPL[((x + 1) * VKL_STENCIL_TRICUBIC_RES * VKL_STENCIL_TRICUBIC_RES + \
       (y + 1) * VKL_STENCIL_TRICUBIC_RES + (z + 1))]

#define __vkl_tricubic_ddx(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x - 1, y, z) + \
           __vkl_tricubic_spl(SPL, x + 1, y, z)))

#define __vkl_tricubic_ddy(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x, y - 1, z) + \
           __vkl_tricubic_spl(SPL, x, y + 1, z)))

#define __vkl_tricubic_ddz(SPL, x, y, z)           \
  (0.5f * (-__vkl_tricubic_spl(SPL, x, y, z - 1) + \
           __vkl_tricubic_spl(SPL, x, y, z + 1)))

#define __vkl_tricubic_d2dxdy(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x - 1, y - 1, z) - \
            __vkl_tricubic_spl(SPL, x - 1, y + 1, z) - \
            __vkl_tricubic_spl(SPL, x + 1, y - 1, z) + \
            __vkl_tricubic_spl(SPL, x + 1, y + 1, z)))

#define __vkl_tricubic_d2dxdz(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x - 1, y, z - 1) - \
            __vkl_tricubic_spl(SPL, x - 1, y, z + 1) - \
            __vkl_tricubic_spl(SPL, x + 1, y, z - 1) + \
            __vkl_tricubic_spl(SPL, x + 1, y, z + 1)))

#define __vkl_tricubic_d2dydz(SPL, x, y, z)            \
  (0.25f * (__vkl_tricubic_spl(SPL, x, y - 1, z - 1) - \
            __vkl_tricubic_spl(SPL, x, y - 1, z + 1) - \
            __vkl_tricubic_spl(SPL, x, y + 1, z - 1) + \
            __vkl_tricubic_spl(SPL, x, y + 1, z + 1)))

#define __vkl_tricubic_d3dxdydz(SPL, x, y, z)                \
  (0.125f * (-__vkl_tricubic_spl(SPL, x - 1, y - 1, z - 1) + \
             __vkl_tricubic_spl(SPL, x - 1, y - 1, z + 1) +  \
             __vkl_tricubic_spl(SPL, x - 1, y + 1, z - 1) -  \
             __vkl_tricubic_spl(SPL, x - 1, y + 1, z + 1) +  \
             __vkl_tricubic_spl(SPL, x + 1, y - 1, z - 1) -  \
             __vkl_tricubic_spl(SPL, x + 1, y - 1, z + 1) -  \
             __vkl_tricubic_spl(SPL, x + 1, y + 1, z - 1) +  \
             __vkl_tricubic_spl(SPL, x + 1, y + 1, z + 1)))

#define __vkl_tricubic_constraints_array(SPL)                               \
  {                                                                         \
    __vkl_tricubic_spl(SPL, 0, 0, 0), __vkl_tricubic_spl(SPL, 0, 0, 1),     \
        __vkl_tricubic_spl(SPL, 0, 1, 0), __vkl_tricubic_spl(SPL, 0, 1, 1), \
        __vkl_tricubic_spl(SPL, 1, 0, 0), __vkl_tricubic_spl(SPL, 1, 0, 1), \
        __vkl_tricubic_spl(SPL, 1, 1, 0), __vkl_tricubic_spl(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddx(SPL, 0, 0, 0), __vkl_tricubic_ddx(SPL, 0, 0, 1), \
        __vkl_tricubic_ddx(SPL, 0, 1, 0), __vkl_tricubic_ddx(SPL, 0, 1, 1), \
        __vkl_tricubic_ddx(SPL, 1, 0, 0), __vkl_tricubic_ddx(SPL, 1, 0, 1), \
        __vkl_tricubic_ddx(SPL, 1, 1, 0), __vkl_tricubic_ddx(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddy(SPL, 0, 0, 0), __vkl_tricubic_ddy(SPL, 0, 0, 1), \
        __vkl_tricubic_ddy(SPL, 0, 1, 0), __vkl_tricubic_ddy(SPL, 0, 1, 1), \
        __vkl_tricubic_ddy(SPL, 1, 0, 0), __vkl_tricubic_ddy(SPL, 1, 0, 1), \
        __vkl_tricubic_ddy(SPL, 1, 1, 0), __vkl_tricubic_ddy(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_ddz(SPL, 0, 0, 0), __vkl_tricubic_ddz(SPL, 0, 0, 1), \
        __vkl_tricubic_ddz(SPL, 0, 1, 0), __vkl_tricubic_ddz(SPL, 0, 1, 1), \
        __vkl_tricubic_ddz(SPL, 1, 0, 0), __vkl_tricubic_ddz(SPL, 1, 0, 1), \
        __vkl_tricubic_ddz(SPL, 1, 1, 0), __vkl_tricubic_ddz(SPL, 1, 1, 1), \
                                                                            \
        __vkl_tricubic_d2dxdy(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dxdy(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d2dxdz(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dxdz(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d2dydz(SPL, 0, 0, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 0, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 1, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 0, 1, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 0, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 0, 1),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 1, 0),                                \
        __vkl_tricubic_d2dydz(SPL, 1, 1, 1),                                \
                                                                            \
        __vkl_tricubic_d3dxdydz(SPL, 0, 0, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 0, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 1, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 0, 1, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 0, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 0, 1),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 1, 0),                              \
        __vkl_tricubic_d3dxdydz(SPL, 1, 1, 1)                               \
  }

// Expanding the integer powers is much faster than using pow!
#define __vkl_tricubic_powers(X)         \
  {                                      \
    1.f, (X), (X) * (X), (X) * (X) * (X) \
  }

// Evaluate a tricubic polynomial given an offset in [0, 1] and coefficients.

// Uniform path.
inline uniform float VdbSampler_tricubicPolynomial(const float *uniform x,
                                                   const float *uniform y,
                                                   const float *uniform z,
                                                   const float *uniform
                                                       coefficients)
{
  unmasked
  {
    // Since there is only one input we vectorize the polynomial
    // evaluation!
    float result = 0.f;
    foreach_tiled(i = 0 ... 4, j = 0 ... 4, k = 0 ... 4)
    {
      result += coefficients[16 * i + 4 * j + k] * x[i] * y[j] * z[k];
    }
    return reduce_add(result);
  }
}

// Varying path.
inline float VdbSampler_tricubicPolynomial(const varying float *uniform x,
                                           const varying float *uniform y,
                                           const varying float *uniform z,
                                           const varying float *uniform
                                               coefficients)
{
  // Expanding the integer powers is much faster than using pow!
  float result = 0.f;
  for (uniform unsigned int i = 0; i < 4; ++i)
    for (uniform unsigned int j = 0; j < 4; ++j)
      for (uniform unsigned int k = 0; k < 4; ++k) {
        result += coefficients[16 * i + 4 * j + k] * x[i] * y[j] * z[k];
      }
  return result;
}

inline vec3f VdbSampler_tricubicPolynomialGradient(
    const varying float *uniform x,
    const varying float *uniform y,
    const varying float *uniform z,
    const varying float *uniform coefficients)
{
  vec3f result = make_vec3f(0.f);
  for (uniform unsigned int i = 0; i < 4; ++i)
    for (uniform unsigned int j = 0; j < 4; ++j)
      for (uniform unsigned int k = 0; k < 4; ++k) {
        const float c = coefficients[16 * i + 4 * j + k];
        if (i > 0)
          result.x += c * i * x[i - 1] * y[j] * z[k];
        if (j > 0)
          result.y += c * j * x[i] * y[j - 1] * z[k];
        if (k > 0)
          result.z += c * k * x[i] * y[j] * z[k - 1];
      }
  return result;
}

// Note: These wrappers are necessary for two reasons; They help clean up the
// code below, and also help avoid nested foreach errors!
inline void VdbSampler_traverseVoxelValuesTricubic(const VdbSampler *uniform
                                                       sampler,
                                                   const uniform vec3i &ic,
                                                   uint64 *uniform voxel,
                                                   vec3ui *uniform domainOffset)
{
  const float time = 0.f;
  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  });
}

inline void VdbSampler_traverseVoxelValuesTricubic(const VdbSampler *uniform
                                                       sampler,
                                                   const vec3i &ic,
                                                   uint64 *uniform voxel,
                                                   vec3ui *uniform domainOffset)
{
  const float time = 0.f;
  __vkl_stencil_dispatch_varying(TRICUBIC, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSampler *uniform sampler,
    const uniform vec3i &ic,
    const uint64 *uniform voxel,
    const vec3ui *uniform domainOffset,
    const uniform float time,
    const uniform uint32 attributeIndex,
    float *uniform sample)
{
  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    const uint64 voxelV        = voxel[tgtIdx];
    const vec3ui domainOffsetV = domainOffset[tgtIdx];
    sample[tgtIdx]             = VdbSampler_sample(
                    sampler, voxelV, domainOffsetV, timeDisp, attributeIndex);
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
    const uint64 *uniform voxel,
    const vec3ui *uniform domainOffset,
    const float &time,
    const uniform uint32 attributeIndex,
    float *uniform sample)
{
  __vkl_stencil_dispatch_varying(TRICUBIC, ic, time, {
    const uint64 voxelV        = voxel[tgtIdx];
    const vec3ui domainOffsetV = domainOffset[tgtIdx];
    sample[tgtIdx]             = VdbSampler_sample(
                    sampler, voxelV, domainOffsetV, timeDisp, attributeIndex);
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSampler *uniform sampler,
    const uniform vec3i &ic,
    const uniform float time,
    const uniform uint32 attributeIndex,
    float *uniform sample)
{
  __vkl_stencil_dispatch_uniform(TRICUBIC, ic, time, {
    sample[tgtIdx] =
        VdbSampler_traverseAndSample(sampler, icDisp, timeDisp, attributeIndex);
  });
}

inline void VdbSampler_computeVoxelValuesTricubic(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
    const float &time,
    const uniform uint32 attributeIndex,
    float *uniform sample)
{
  __vkl_stencil_dispatch_varying(TRICUBIC, ic, time, {
    sample[tgtIdx] =
        VdbSampler_traverseAndSample(sampler, icDisp, timeDisp, attributeIndex);
  });
}

// Single attribute uniform.
inline uniform float VdbSampler_interpolateTricubic(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform float time,
    const uniform uint32 attributeIndex)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform float sample[VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_computeVoxelValuesTricubic(
      sampler, ic, time, attributeIndex, sample);

  const uniform float constraints[]  = __vkl_tricubic_constraints_array(sample);
  const uniform float coefficients[] = __vkl_tricubic_coefficients(constraints);
  const uniform vec3f delta          = indexCoordinates - make_vec3f(ic);
  const uniform float x[]            = __vkl_tricubic_powers(delta.x);
  const uniform float y[]            = __vkl_tricubic_powers(delta.y);
  const uniform float z[]            = __vkl_tricubic_powers(delta.z);
  return VdbSampler_tricubicPolynomial(x, y, z, coefficients);
}

// Single attribute varying.
inline float VdbSampler_interpolateTricubic(const VdbSampler *uniform sampler,
                                            const vec3f &indexCoordinates,
                                            const float &time,
                                            const uniform uint32 attributeIndex)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uniform float sample[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_computeVoxelValuesTricubic(
      sampler, ic, time, attributeIndex, sample);

  const varying float *uniform s = ((const varying float *uniform) & sample[0]);
  const float constraints[]      = __vkl_tricubic_constraints_array(s);
  const float coefficients[]     = __vkl_tricubic_coefficients(constraints);
  const vec3f delta              = indexCoordinates - make_vec3f(ic);
  const float x[]                = __vkl_tricubic_powers(delta.x);
  const float y[]                = __vkl_tricubic_powers(delta.y);
  const float z[]                = __vkl_tricubic_powers(delta.z);
  return VdbSampler_tricubicPolynomial(x, y, z, coefficients);
}

// Single attribute stream.
inline void VdbSampler_interpolateTricubic(const VdbSampler *uniform sampler,
                                           const uniform unsigned int N,
                                           const vec3f *uniform
                                               objectCoordinates,
                                           const float *uniform times,
                                           const uint32 uniform attributeIndex,
                                           float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const float time             = times ? times[i] : 0.f;
    samples[i]                   = VdbSampler_interpolateTricubic(
                          sampler, indexCoordinates, time, attributeIndex);
  }
}

// Multi attribute uniform.
inline void VdbSampler_interpolateTricubic(
    const VdbSampler *uniform sampler,
    const uniform vec3f &indexCoordinates,
    const uniform float time,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  const uniform vec3i ic = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));

  uniform uint64 voxel[VKL_STENCIL_TRICUBIC_SIZE];
  uniform vec3ui domainOffset[VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_traverseVoxelValuesTricubic(sampler, ic, voxel, domainOffset);

  const uniform vec3f delta = indexCoordinates - make_vec3f(ic);
  const uniform float x[]   = __vkl_tricubic_powers(delta.x);
  const uniform float y[]   = __vkl_tricubic_powers(delta.y);
  const uniform float z[]   = __vkl_tricubic_powers(delta.z);

  for (uniform unsigned int a = 0; a < M; a++) {
    uniform float sample[VKL_STENCIL_TRICUBIC_SIZE];
    VdbSampler_computeVoxelValuesTricubic(
        sampler, ic, voxel, domainOffset, time, attributeIndices[a], sample);

    const uniform float constraints[] =
        __vkl_tricubic_constraints_array(sample);
    const uniform float coefficients[] =
        __vkl_tricubic_coefficients(constraints);
    samples[a] = VdbSampler_tricubicPolynomial(x, y, z, coefficients);
  }
}

// Multi attribute varying.
inline void VdbSampler_interpolateTricubic(const VdbSampler *uniform sampler,
                                           const vec3f &indexCoordinates,
                                           const float &time,
                                           const uniform uint32 M,
                                           const uint32 *uniform
                                               attributeIndices,
                                           float *uniform samples)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uniform uint64 voxel[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
  uniform vec3ui domainOffset[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_traverseVoxelValuesTricubic(sampler, ic, voxel, domainOffset);

  const vec3f delta = indexCoordinates - make_vec3f(ic);
  const float x[]   = __vkl_tricubic_powers(delta.x);
  const float y[]   = __vkl_tricubic_powers(delta.y);
  const float z[]   = __vkl_tricubic_powers(delta.z);

  for (uniform unsigned int a = 0; a < M; a++) {
    uniform float sample[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
    VdbSampler_computeVoxelValuesTricubic(
        sampler, ic, voxel, domainOffset, time, attributeIndices[a], sample);

    const varying float *uniform s = (const varying float *uniform) & sample;
    const float constraints[]      = __vkl_tricubic_constraints_array(s);
    const float coefficients[]     = __vkl_tricubic_coefficients(constraints);
    samples[a * VKL_TARGET_WIDTH + programIndex] =
        VdbSampler_tricubicPolynomial(x, y, z, coefficients);
  }
}

// Multi attribute stream.
inline void VdbSampler_interpolateTricubic(
    const VdbSampler *uniform sampler,
    const uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
    const uniform uint32 M,
    const uint32 *uniform attributeIndices,
    float *uniform samples)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const vec3i ic               = make_vec3i(floor(indexCoordinates.x),
                                              floor(indexCoordinates.y),
                                              floor(indexCoordinates.z));

    uniform uint64 voxel[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
    uniform vec3ui domainOffset[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
    VdbSampler_traverseVoxelValuesTricubic(sampler, ic, voxel, domainOffset);

    const float time  = times ? times[i] : 0.f;
    const vec3f delta = indexCoordinates - make_vec3f(ic);
    const float x[]   = __vkl_tricubic_powers(delta.x);
    const float y[]   = __vkl_tricubic_powers(delta.y);
    const float z[]   = __vkl_tricubic_powers(delta.z);

    for (uniform unsigned int a = 0; a < M; a++) {
      uniform float sample[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
      VdbSampler_computeVoxelValuesTricubic(
          sampler, ic, voxel, domainOffset, time, attributeIndices[a], sample);

      const varying float *uniform s = (const varying float *uniform) & sample;
      const float constraints[]      = __vkl_tricubic_constraints_array(s);
      const float coefficients[]     = __vkl_tricubic_coefficients(constraints);
      samples[i * M + a] = VdbSampler_tricubicPolynomial(x, y, z, coefficients);
    }
  }
}

// Gradient varying.
inline vec3f VdbSampler_computeGradientTricubic(
    const VdbSampler *uniform sampler,
    const vec3f &indexCoordinates,
    const float &time,
    const uniform uint32 attributeIndex)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  uniform float sample[VKL_TARGET_WIDTH * VKL_STENCIL_TRICUBIC_SIZE];
  VdbSampler_computeVoxelValuesTricubic(
      sampler, ic, time, attributeIndex, sample);

  const varying float *uniform s = ((const varying float *uniform) & sample[0]);
  const float constraints[]      = __vkl_tricubic_constraints_array(s);
  const float coefficients[]     = __vkl_tricubic_coefficients(constraints);
  const vec3f delta              = indexCoordinates - make_vec3f(ic);
  const float x[]                = __vkl_tricubic_powers(delta.x);
  const float y[]                = __vkl_tricubic_powers(delta.y);
  const float z[]                = __vkl_tricubic_powers(delta.z);
  return VdbSampler_tricubicPolynomialGradient(x, y, z, coefficients);
}

// Gradient stream.
inline void VdbSampler_computeGradientTricubic(
    const VdbSampler *uniform sampler,
    uniform unsigned int N,
    const vec3f *uniform objectCoordinates,
    const float *uniform times,
    const uniform uint32 attributeIndex,
    vec3f *uniform gradients)
{
  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);
    const float time             = times ? times[i] : 0.f;
    const vec3f gradient         = VdbSampler_computeGradientTricubic(
                sampler, indexCoordinates, time, attributeIndex);
    // Note: xfmNormal takes inverse!
    gradients[i] = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

// This macro centralizes filter dispatch for the public API.
// To add a filter, add the enum and than add a case here.
// Then, implement all of the filter-specific functions above.
#define __vkl_switch_filter(FilterEnum, F, ...)            \
  {                                                        \
    switch (FilterEnum) {                                  \
    case VKL_FILTER_TRILINEAR: {                           \
      F##Trilinear(__VA_ARGS__);                           \
      break;                                               \
    }                                                      \
    case VKL_FILTER_TRICUBIC: {                            \
      F##Tricubic(__VA_ARGS__);                            \
      break;                                               \
    }                                                      \
    default: {                                             \
      assert(sampler->super.filter == VKL_FILTER_NEAREST); \
      F##Nearest(__VA_ARGS__);                             \
      break;                                               \
    }                                                      \
    }                                                      \
  }

/*
 * Special case: we know that coordinates are uniform.
 */
export void EXPORT_UNIQUE(VdbSampler_computeSample_uniform,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform time,
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

  __vkl_switch_filter(sampler->super.filter,
                      *sample = VdbSampler_interpolate,
                      sampler,
                      indexCoordinates,
                      *time,
                      attributeIndex);
}

export void EXPORT_UNIQUE(VdbSampler_computeSample,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
                          const uniform uint32 attributeIndex,
                          void *uniform _samples)
{
  if (imask[programIndex]) {
    const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
    assert(sampler);
    assert(sampler->grid);

    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;

    const varying float *uniform time = (const varying float *uniform)_time;
    varying float *uniform sample     = (varying float *uniform)_samples;

    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    __vkl_switch_filter(sampler->super.filter,
                        *sample = VdbSampler_interpolate,
                        sampler,
                        indexCoordinates,
                        *time,
                        attributeIndex);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM_uniform,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
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

  const float *uniform time = (const float *uniform)_time;

  __vkl_switch_filter(sampler->super.filter,
                      VdbSampler_interpolate,
                      sampler,
                      indexCoordinates,
                      *time,
                      M,
                      attributeIndices,
                      samples);
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
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
    const varying float *uniform time = (const varying float *uniform)_time;

    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    __vkl_switch_filter(sampler->super.filter,
                        VdbSampler_interpolate,
                        sampler,
                        indexCoordinates,
                        *time,
                        M,
                        attributeIndices,
                        samples);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSample_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const float *uniform time,
                          const uniform uint32 attributeIndex,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  __vkl_switch_filter(sampler->super.filter,
                      VdbSampler_interpolate,
                      sampler,
                      N,
                      objectCoordinates,
                      time,
                      attributeIndex,
                      samples);
}

export void EXPORT_UNIQUE(VdbSampler_computeSampleM_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const float *uniform time,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  __vkl_switch_filter(sampler->super.filter,
                      VdbSampler_interpolate,
                      sampler,
                      N,
                      objectCoordinates,
                      time,
                      M,
                      attributeIndices,
                      samples);
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
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
    const varying float *uniform time = (const varying float *uniform)_time;

    vec3f gradient;
    __vkl_switch_filter(sampler->super.gradientFilter,
                        gradient = VdbSampler_computeGradient,
                        sampler,
                        indexCoordinates,
                        *time,
                        attributeIndex);

    // Note: xfmNormal takes inverse!
    *gradients = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const float *uniform time,
                          const uniform uint32 attributeIndex,
                          vec3f *uniform gradients)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  __vkl_switch_filter(sampler->super.gradientFilter,
                      VdbSampler_computeGradient,
                      sampler,
                      N,
                      objectCoordinates,
                      time,
                      attributeIndex,
                      gradients);
}

// -----------------------------------------------------------------------------
// Interface for iterators
// -----------------------------------------------------------------------------

inline float VdbSampler_iterator_computeSample_varying(
    const Sampler *uniform _sampler,
    const varying vec3f &objectCoordinates,
    const varying float &time)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, objectCoordinates);

  const uniform uint32 attributeIndex = 0;

  float sample = 0.f;

  __vkl_switch_filter(sampler->super.filter,
                      sample = VdbSampler_interpolate,
                      sampler,
                      indexCoordinates,
                      time,
                      attributeIndex);

  return sample;
}

// -----------------------------------------------------------------------------
// Construction.
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

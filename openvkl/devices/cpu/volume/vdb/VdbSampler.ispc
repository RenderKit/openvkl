// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "VdbGrid.h"
#include "VdbSampler.ih"
#include "VdbSampler_filter.ih"
#include "VdbSampler_leafHandler.ih"
#include "VdbVolume.ih"
#include "common/export_util.h"

// ---------------------------------------------------------------------------
// Sampling.
// ---------------------------------------------------------------------------

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

// ---------------------------------------------------------------------------
// Gradient computation.
// ---------------------------------------------------------------------------

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

float VdbSampler_iterator_computeSample_varying(
    const Sampler *uniform _sampler,
    const varying vec3f &objectCoordinates,
    const uniform uint32 attributeIndex,
    const varying float &time)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, objectCoordinates);

  float sample = 0.f;

  __vkl_switch_filter(sampler->super.filter,
                      sample = VdbSampler_interpolate,
                      sampler,
                      indexCoordinates,
                      time,
                      attributeIndex);

  return sample;
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
  const uniform uint64 leafIndex     = vklVdbVoxelLeafGetIndex(voxel);

  __vkl_vdb_leaf_handler(
      return VdbSampler_computeValueRange,
             voxelType,
             vklVdbVoxelLeafGetFormat(voxel),
             grid->allLeavesConstant,
             vklVdbVoxelLeafGetTemporalFormat(voxel),
             grid,
             leafIndex,
             vklVdbGetLeafDataIndex(grid, leafIndex, attributeIndex),
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

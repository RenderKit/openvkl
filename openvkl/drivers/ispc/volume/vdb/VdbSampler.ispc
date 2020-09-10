// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "../common/Data.ih"
#include "VdbLeafAccessObserver.ih"
#include "VdbSampler.ih"
#include "VdbVolume.ih"
#include "common/export_util.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.ih"

// ---------------------------------------------------------------------------
// The main entrypoint for sampling a volume.
// This is called from the interpolation scheduling routines below.
// ---------------------------------------------------------------------------

inline varying float VdbSampler_sample(const VdbSampler *uniform sampler,
                                       const varying vec3i &ic)
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

  return VdbSampler_dispatchInner_uniform_0(sampler, domainOffset, 0);
}

// ---------------------------------------------------------------------------
// Value range computation.
// ---------------------------------------------------------------------------

inline void extendValueRangeFilterFloat(const void *uniform _grid,
                                        const vec3ui *uniform offset,
                                        uint32 uniform level,
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
    const x = res;
    const float sample =
        VdbSampler_sample(&sampler, make_vec3i(x + os.x, y + os.y, z + os.z));
    vminFilter = min(vminFilter, sample);
    vmaxFilter = max(vmaxFilter, sample);
  }
  // We only have to go to x = res+radius as we added those voxels above.
  foreach (z = -radius... res + radius, x = -radius... res) {
    const y = res;
    const float sample =
        VdbSampler_sample(&sampler, make_vec3i(x + os.x, y + os.y, z + os.z));
    vminFilter = min(vminFilter, sample);
    vmaxFilter = max(vmaxFilter, sample);
  }
  // We only have to go to x = res+radius and y = res+radius as we added those
  // voxels above.
  foreach (y = -radius... res, x = -radius... res) {
    const z = res;
    const float sample =
        VdbSampler_sample(&sampler, make_vec3i(x + os.x, y + os.y, z + os.z));
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
                          const Data1D *uniform data,
                          const vec3ui *uniform offset,
                          uint32 uniform level,
                          uniform box1f *uniform range)
{
  range->lower = get_float(*data, (uint32)0);
  range->upper = range->lower;
  extendValueRangeFilterFloat(_grid, offset, level, range);
}

/*
 * Compute the value range on the given constant float leaf.
 */
export void EXPORT_UNIQUE(VdbSampler_valueRangeConstantFloat,
                          const void *uniform _grid,
                          const Data1D *uniform data,
                          const vec3ui *uniform offset,
                          uint32 uniform level,
                          uniform box1f *uniform range)
{
  const uniform uint32 numVoxels = vklVdbLevelNumVoxels(level);
  // As suggested in the ISPC performance guide, we perform min/max computation
  // per lane and only reduce across lanes in the end.
  float vmin = pos_inf;
  float vmax = neg_inf;
  foreach (i = 0 ... numVoxels) {
    vmin = min(vmin, get_float(*data, (uint32)i));
    vmax = max(vmax, get_float(*data, (uint32)i));
  }
  range->lower = reduce_min(vmin);
  range->upper = reduce_max(vmax);

  extendValueRangeFilterFloat(_grid, offset, level, range);
}

// ---------------------------------------------------------------------------
// Interpolation.
// ---------------------------------------------------------------------------

/*
 * Nearest neighbor interpolation is the fastest version, but also gives
 * blocky results. This should be good for indirect light etc.
 */
inline float VdbSampler_interpolateNearest_varying(
    const VdbSampler *uniform sampler, const vec3f &indexCoordinates)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  return VdbSampler_sample(sampler, ic);
}

inline uniform float VdbSampler_interpolateNearest_uniform(
    const VdbSampler *uniform sampler, const uniform vec3f &indexCoordinates)
{
  const vec3f ic = indexCoordinates;
  const float sample = VdbSampler_interpolateNearest_varying(sampler, ic);
  return extract(sample, 0);
}

/*
 * Compute voxel values for the eight corners required in trilinear
 * interpolation.
 * This is used for both sampling and gradient computation!
 */
inline void VdbSampler_computeVoxelValuesTrilinear(
    const VdbSampler *uniform sampler,
    const vec3i &ic,
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
          VdbSampler_sample(sampler, coord);
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
            VdbSampler_sample(sampler, coord);
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
            VdbSampler_sample(sampler, coord);
      }
    }
  }
}

/*
 * Trilinear sampling is a good default for directly visible volumes.
 * The implementation is optimized to exploit SIMD.
 */
inline varying float VdbSampler_interpolateTrilinear_varying(
    const VdbSampler *uniform sampler, const vec3f &indexCoordinates)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(sampler, ic, sample);

  const varying float *uniform s = (const varying float *uniform) & sample;
  return lerp(
      delta.x,
      lerp(delta.y, lerp(delta.z, s[0], s[1]), lerp(delta.z, s[2], s[3])),
      lerp(delta.y, lerp(delta.z, s[4], s[5]), lerp(delta.z, s[6], s[7])));
}

/*
 * Uniform path. This allows us to skip the selection magic in the function
 * above if we know that there is only one query.
 */
inline uniform float VdbSampler_interpolateTrilinear_uniform(
    const VdbSampler *uniform sampler, const uniform vec3f &indexCoordinates)
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
      sample[o] = VdbSampler_sample(sampler, coord);
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

/*
 * Gradients in piecewise constant fields are zero (almost everywhere, we'll
 * say everywhere...)
 */
inline vec3f VdbSampler_computeGradientNearest(
    const VdbSampler *uniform sampler, const vec3f &indexCoordinates)
{
  return make_vec3f(0.f);
}

/*
 * Gradients in trilinear fields.
 */
inline vec3f VdbSampler_computeGradientTrilinear(
    const VdbSampler *uniform sampler, const vec3f &indexCoordinates)
{
  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  uniform float sample[VKL_TARGET_WIDTH * 8];
  VdbSampler_computeVoxelValuesTrilinear(sampler, ic, sample);

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
                          void *uniform _samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  float *uniform samples = (float *uniform)_samples;

  const uniform vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

  *samples = sampler->interpolate_uniform(sampler, indexCoordinates);
}

export void EXPORT_UNIQUE(VdbSampler_computeSample,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
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

    *samples = sampler->interpolate_varying(sampler, indexCoordinates);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeSample_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          float *uniform samples)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    samples[i] = sampler->interpolate_varying(sampler, indexCoordinates);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
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
    const vec3f gradient =
        sampler->computeGradient_varying(sampler, indexCoordinates);

    // Note: xfmNormal takes inverse!
    *gradients = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

export void EXPORT_UNIQUE(VdbSampler_computeGradient_stream,
                          const void *uniform _sampler,
                          uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          vec3f *uniform gradients)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  foreach (i = 0 ... N) {
    const vec3f oc               = objectCoordinates[i];
    const vec3f indexCoordinates = xfmPoint(sampler->grid->objectToIndex, oc);

    const vec3f gradient =
        sampler->computeGradient_varying(sampler, indexCoordinates);

    // Note: xfmNormal takes inverse!
    gradients[i] = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

// -----------------------------------------------------------------------------
// Interface for iterators
// -----------------------------------------------------------------------------

inline float VdbSampler_iterator_computeSample_varying(
    const Sampler *uniform _sampler, const varying vec3f &objectCoordinates)
{
  const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
  assert(sampler);
  assert(sampler->grid);

  const vec3f indexCoordinates =
      xfmPoint(sampler->grid->objectToIndex, objectCoordinates);

  return sampler->interpolate_varying(sampler, indexCoordinates);
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
                          uniform vkl_uint32 maxSamplingDepth)
{
  VdbSampler *uniform sampler = (VdbSampler * uniform) _sampler;
  CALL_ISPC(Sampler_setFilters, &sampler->super, filter, gradientFilter);

  // For hit iterators.
  sampler->super.computeSample_varying =
      VdbSampler_iterator_computeSample_varying;

  sampler->maxSamplingDepth = maxSamplingDepth;

  switch (filter) {
  case VKL_FILTER_NEAREST:
    sampler->interpolate_uniform = VdbSampler_interpolateNearest_uniform;
    sampler->interpolate_varying = VdbSampler_interpolateNearest_varying;
    break;
  case VKL_FILTER_TRILINEAR:
    sampler->interpolate_uniform = VdbSampler_interpolateTrilinear_uniform;
    sampler->interpolate_varying = VdbSampler_interpolateTrilinear_varying;
    break;
  }

  switch (gradientFilter) {
  case VKL_FILTER_NEAREST:
    sampler->computeGradient_varying = VdbSampler_computeGradientNearest;
    break;
  case VKL_FILTER_TRILINEAR:
    sampler->computeGradient_varying = VdbSampler_computeGradientTrilinear;
    break;
  }
}

export void EXPORT_UNIQUE(VdbSampler_destroy, void *uniform _sampler)
{
  VdbSampler *uniform sampler = (VdbSampler * uniform) _sampler;
  delete sampler;
}


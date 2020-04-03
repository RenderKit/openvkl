// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "VdbVolume.ih"
#include "common/export_util.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.ih"
/*
 * Compute the value range on the given constant float leaf.
 */
export void EXPORT_UNIQUE(VdbSampler_valueRangeConstantFloat,
                          const uniform float *uniform data,
                          uniform uint32 numVoxels,
                          uniform box1f *uniform range)
{
  // As suggested in the ISPC performance guide, we perform min/max computation
  // per lane and only reduce across lanes in the end.
  float vmin = pos_inf;
  float vmax = neg_inf;
  foreach (i = 0 ... numVoxels) {
    vmin = min(vmin, data[i]);
    vmax = max(vmax, data[i]);
  }
  range->lower = reduce_min(vmin);
  range->upper = reduce_max(vmax);
}

// ---------------------------------------------------------------------------
// The main entrypoint for sampling a volume.
// This is called from the interpolation scheduling routines below.
// ---------------------------------------------------------------------------

inline varying float VdbSampler_sample(const VdbGrid *uniform grid,
                                       const varying vec3i &ic)
{
  assert(grid->levels[0].numNodes == 1);

  const vec3i rootOrg = grid->rootOrigin;
  if (ic.x < rootOrg.x || ic.y < rootOrg.y || ic.z < rootOrg.z)
    return 0.f;

  const vec3ui domainOffset = make_vec3ui(ic - rootOrg);
  if (domainOffset.x >= VKL_VDB_RES_0 || domainOffset.y >= VKL_VDB_RES_0 ||
      domainOffset.z >= VKL_VDB_RES_0) {
    return 0.f;
  }

  return VdbSampler_dispatchInner_uniform_0(grid, domainOffset, 0);
}

// ---------------------------------------------------------------------------
// Interpolation.
// ---------------------------------------------------------------------------

/*
 * Nearest neighbor interpolation is the fastest version, but also gives
 * blocky results. This should be good for indirect light etc.
 */
float VdbSampler_computeSampleNearest(const uniform VdbGrid *uniform grid,
                                      const varying vec3f &indexCoordinates)
{
  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  return VdbSampler_sample(grid, ic);
}

/*
 * Trilinear sampling is a good default for directly visible volumes.
 * The implementation is optimized to exploit SIMD.
 */
float VdbSampler_computeSampleTrilinear(const uniform VdbGrid *uniform grid,
                                        const varying vec3f &indexCoordinates)
{
  const vec3i ic      = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta   = indexCoordinates - make_vec3f(ic);
  const vec3f omdelta = make_vec3f(1.f) - delta;

  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  uniform float sample[VKL_TARGET_WIDTH * 8];

  // The goal of this code is to keep as many lanes busy as possible.
  // The first case is that we have as many queries as there are
  // lanes, so we need not do anything smart (=expensive), no lane will
  // be idle.
  if (lanemask() == ((1 << VKL_TARGET_WIDTH) - 1)) {
    for (uniform unsigned int i = 0; i < 8; ++i) {
      const vec3i coord                       = ic + offset[i];
      sample[i * VKL_TARGET_WIDTH + programIndex] = VdbSampler_sample(grid, coord);
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
            VdbSampler_sample(grid, coord);
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
        sample[o * VKL_TARGET_WIDTH + instance] = VdbSampler_sample(grid, coord);
      }
    }
  }

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
uniform float VdbSampler_computeSampleTrilinear_uniform(
    const uniform VdbGrid *uniform grid, const uniform vec3f &indexCoordinates)
{
  const uniform vec3i ic      = make_vec3i(floor(indexCoordinates.x),
                                      floor(indexCoordinates.y),
                                      floor(indexCoordinates.z));
  const uniform vec3f delta   = indexCoordinates - make_vec3f(ic);
  const uniform vec3f omdelta = make_vec3f(1.f) - delta;

  static const uniform vec3i offset[] = {{0, 0, 0},
                                         {0, 0, 1},
                                         {0, 1, 0},
                                         {0, 1, 1},
                                         {1, 0, 0},
                                         {1, 0, 1},
                                         {1, 1, 0},
                                         {1, 1, 1}};

  unmasked
  {
    uniform float sample[8];
    foreach (o = 0 ... 8) {
      const vec3i coord = make_vec3i(
          ic.x + offset[o].x, ic.y + offset[o].y, ic.z + offset[o].z);
      sample[o] = VdbSampler_sample(grid, coord);
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

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

export void EXPORT_UNIQUE(VdbSampler_computeSample,
                          uniform const int *uniform imask,
                          const void *uniform _volume,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  VdbVolume *uniform volume   = (VdbVolume * uniform) _volume;
  const VdbGrid *uniform grid = volume->grid;
  assert(grid);

  const uniform VKLFilter filter = grid->filter;

  const varying vec3f *uniform objectCoordinates =
      (const varying vec3f *uniform)_objectCoordinates;
  varying float *uniform samples = (varying float *uniform)_samples;

  const vec3f indexCoordinates =
      xfmPoint(grid->objectToIndex, *objectCoordinates);

  switch (filter) {
  case VKL_FILTER_NEAREST:
    if (imask[programIndex])
      *samples = VdbSampler_computeSampleNearest(grid, indexCoordinates);
    break;

  case VKL_FILTER_TRILINEAR:
    if (imask[programIndex])
      *samples = VdbSampler_computeSampleTrilinear(grid, indexCoordinates);
    break;

  default:
    *samples = 0.f;
    break;
  }
}

/*
 * Special case: we know that coordinates are uniform.
 */
export void EXPORT_UNIQUE(VdbSampler_computeSample_uniform,
                          const void *uniform _volume,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  VdbVolume *uniform volume   = (VdbVolume * uniform) _volume;
  const VdbGrid *uniform grid = volume->grid;
  assert(grid);

  const uniform VKLFilter filter = grid->filter;

  const uniform vec3f *uniform objectCoordinates =
      (const uniform vec3f *uniform)_objectCoordinates;
  uniform float *uniform samples = (uniform float *uniform)_samples;

  const uniform vec3f indexCoordinates =
      xfmPoint(grid->objectToIndex, *objectCoordinates);

  switch (filter) {
  case VKL_FILTER_NEAREST: {
    *samples = extract(VdbSampler_computeSampleNearest(
                           grid, ((varying vec3f)indexCoordinates)),
                       0);
    break;
  }

  case VKL_FILTER_TRILINEAR:
    *samples =
        VdbSampler_computeSampleTrilinear_uniform(grid, indexCoordinates);
    break;

  default:
    *samples = 0.f;
    break;
  }
}

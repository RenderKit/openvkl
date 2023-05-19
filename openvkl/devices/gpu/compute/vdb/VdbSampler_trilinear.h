// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_stencilDispatch.h"
#include "VdbSampler_traverseAndSample.h"

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
    const VdbSamplerShared *sampler,
    const vec3i &ic,
    const float &time,
    const uint32 attributeIndex,
    const VKLFeatureFlags featureFlags,
    float *sample)  // Array of 8 elements!
{
  assert(!sampler->grid->dense);

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
    sample[tgtIdx] = VdbSampler_traverseAndSample(
        sampler, icDisp, timeDisp, attributeIndex, featureFlags);
  });
}

/*
 * Uniform path. This allows us to skip the selection magic in the function
 * above if we know that there is only one query.
 */
inline float VdbSampler_interpolateTrilinear(const VdbSamplerShared *sampler,
                                             const vec3f &indexCoordinates,
                                             const float time,
                                             const uint32 attributeIndex,
                                             const VKLFeatureFlags featureFlags)
{
  assert(!sampler->grid->dense);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));

  float sample[8];

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
    sample[tgtIdx] = VdbSampler_traverseAndSample(
        sampler, icDisp, timeDisp, attributeIndex, featureFlags);
  });

  const vec3f delta = indexCoordinates - make_vec3f(ic);
  return lerp(delta.x,
              lerp(delta.y,
                   lerp(delta.z, sample[0], sample[1]),
                   lerp(delta.z, sample[2], sample[3])),
              lerp(delta.y,
                   lerp(delta.z, sample[4], sample[5]),
                   lerp(delta.z, sample[6], sample[7])));
}

inline void VdbSampler_interpolateTrilinear(const VdbSamplerShared *sampler,
                                            const vec3f &indexCoordinates,
                                            const float time,
                                            const uint32 M,
                                            const uint32 *attributeIndices,
                                            const VKLFeatureFlags featureFlags,
                                            float *samples)
{
  assert(!sampler->grid->dense);

  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);

  uint64 voxel[8];
  vec3ui domainOffset[8];

  __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
    uint64 voxelV;
    vec3ui domainOffsetV;
    VdbSampler_traverse(sampler, icDisp, voxelV, domainOffsetV);

    voxel[tgtIdx]        = voxelV;
    domainOffset[tgtIdx] = domainOffsetV;
  });

  for (unsigned int a = 0; a < M; a++) {
    float sample[8];

    __vkl_stencil_dispatch_uniform(TRILINEAR, ic, time, {
      const uint64 voxelV        = voxel[tgtIdx];
      const vec3ui domainOffsetV = domainOffset[tgtIdx];
      sample[tgtIdx]             = VdbSampler_sample(sampler,
                                         voxelV,
                                         domainOffsetV,
                                         timeDisp,
                                         attributeIndices[a],
                                         featureFlags);
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

/*
 * Gradients in trilinear fields.
 */
inline vec3f VdbSampler_computeGradientTrilinear(
    const VdbSamplerShared *sampler,
    const vec3f &indexCoordinates,
    const float &time,
    const uint32 attributeIndex,
    const VKLFeatureFlags featureFlags)
{
  assert(!sampler->grid->dense);

  const vec3i ic    = make_vec3i(floor(indexCoordinates.x),
                              floor(indexCoordinates.y),
                              floor(indexCoordinates.z));
  const vec3f delta = indexCoordinates - make_vec3f(ic);
  float s[8];
  VdbSampler_computeVoxelValuesTrilinear(
      sampler, ic, time, attributeIndex, featureFlags, s);

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

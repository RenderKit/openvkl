// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_stencilDispatch.h"
#include "VdbSampler_traverseAndSample.h"

// ---------------------------------------------------------------------------
// NEAREST interpolation.
// ---------------------------------------------------------------------------

// for vertex-centered data, we need to offset the index coordinate by one half
// voxel for correct results.
inline float VdbSampler_getNearestIndexOffset(const VdbSamplerShared *sampler)
{
  return sampler->grid->constantCellData ? 0.f : 0.5f;
}

/*
 * Nearest neighbor interpolation is the fastest version, but also gives
 * blocky results. This should be good for indirect light etc.
 */

inline float VdbSampler_interpolateNearest(const VdbSamplerShared *sampler,
                                           const vec3f &indexCoordinates,
                                           const float time,
                                           const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);

  const float offset = VdbSampler_getNearestIndexOffset(sampler);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x + offset),
                              floor(indexCoordinates.y + offset),
                              floor(indexCoordinates.z + offset));

  return VdbSampler_traverseAndSample(sampler, ic, time, attributeIndex);
}

inline void VdbSampler_interpolateNearest(const VdbSamplerShared *sampler,
                                          const vec3f &indexCoordinates,
                                          const float time,
                                          const uint32 M,
                                          const uint32 *attributeIndices,
                                          float *samples)
{
  assert(!sampler->grid->dense);

  const float offset = VdbSampler_getNearestIndexOffset(sampler);

  const vec3i ic = make_vec3i(floor(indexCoordinates.x + offset),
                              floor(indexCoordinates.y + offset),
                              floor(indexCoordinates.z + offset));

  uint64 voxel;
  vec3ui domainOffset;
  VdbSampler_traverse(sampler, ic, voxel, domainOffset);

  for (unsigned int a = 0; a < M; a++) {
    samples[a] = VdbSampler_sample(
        sampler, voxel, domainOffset, time, attributeIndices[a]);
  }
}

/*
 * Gradients in piecewise constant fields are zero (almost everywhere, we'll
 * say everywhere...)
 */
inline vec3f VdbSampler_computeGradientNearest(const VdbSamplerShared *sampler,
                                               const vec3f &indexCoordinates,
                                               const float &time,
                                               const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);

  return make_vec3f(0.f);
}

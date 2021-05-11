// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/vdb.h>
#include "VdbGrid.h"
#include "VdbVolume.ih"
#include "VdbSampler.ih"
#include "VdbSampler_filter.ih"
#include "common/export_util.h"


// -----------------------------------------------------------------------------
// Interface for iterators
// -----------------------------------------------------------------------------

float VdbSampler_iterator_computeSample_varying(
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

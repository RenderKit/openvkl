// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"

#include "Sampler.ih"

export void EXPORT_UNIQUE(Sampler_create,
                          const void* uniform _volume,
                          void* uniform _sampler)
{
  SamplerShared *uniform sampler = (SamplerShared *uniform)_sampler;
  memset(sampler, 0, sizeof(uniform SamplerShared));
  sampler->volume = (const VolumeShared *uniform) _volume;
}

export void EXPORT_UNIQUE(Sampler_setFilters,
                          void *uniform _sampler,
                          uniform VKLFilter filter,
                          uniform VKLFilter gradientFilter)
{
  SamplerShared *uniform sampler = (SamplerShared *uniform)_sampler;
  sampler->filter = filter;
  sampler->gradientFilter = gradientFilter;
}

export void EXPORT_UNIQUE(Sampler_destroy,
                          void *uniform _sampler)
{
  SamplerShared *uniform sampler = (SamplerShared *uniform)_sampler;
  delete sampler;
}

// these functions do not yet support multi-attribute or time-varying volumes.

export void EXPORT_UNIQUE(Sampler_sample_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          float *uniform samples)
{
  SamplerShared *uniform self = (SamplerShared * uniform) _self;

  varying float time = 0.f;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    samples[i]       = self->computeSample_varying(self, oc, 0, time);
  }
}

export void EXPORT_UNIQUE(Sampler_gradient_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          vec3f *uniform gradients)
{
  SamplerShared *uniform self = (SamplerShared * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    gradients[i]     = self->computeGradient_varying(self, oc);
  }
}

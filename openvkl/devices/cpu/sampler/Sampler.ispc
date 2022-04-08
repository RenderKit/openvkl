// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Sampler.ih"

export Sampler *uniform EXPORT_UNIQUE(Sampler_create,
                                      const void* uniform _volume)
{
  Sampler *uniform sampler = uniform new Sampler;
  memset(sampler, 0, sizeof(uniform Sampler));
  sampler->volume = (const Volume *uniform) _volume;
  return sampler;
}

export void EXPORT_UNIQUE(Sampler_setFilters,
                          void *uniform _sampler,
                          uniform VKLFilter filter,
                          uniform VKLFilter gradientFilter)
{
  Sampler *uniform sampler = (Sampler *uniform)_sampler;
  sampler->filter = filter;
  sampler->gradientFilter = gradientFilter;
}

export void EXPORT_UNIQUE(Sampler_destroy,
                          void *uniform _sampler)
{
  Sampler *uniform sampler = (Sampler *uniform)_sampler;
  delete sampler;
}

// these functions do not yet support multi-attribute or time-varying volumes.

export void EXPORT_UNIQUE(Sampler_sample_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          float *uniform samples)
{
  Sampler *uniform self = (Sampler * uniform) _self;

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
  Sampler *uniform self = (Sampler * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    gradients[i]     = self->computeGradient_varying(self, oc);
  }
}

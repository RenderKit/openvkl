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

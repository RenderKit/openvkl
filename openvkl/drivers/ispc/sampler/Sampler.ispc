// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Sampler.ih"

export Sampler *uniform EXPORT_UNIQUE(Sampler_create)
{
  Sampler *uniform sampler = uniform new Sampler;
  return sampler;
}

export void EXPORT_UNIQUE(Sampler_set,
                          void *uniform _sampler,
                          void *uniform _volume,
                          uniform VKLFilter filter,
                          uniform VKLFilter gradientFilter)
{
  Sampler *uniform sampler = (Sampler *uniform)_sampler;
  sampler->volume = (Volume *uniform)_volume;
  sampler->filter = filter;
  sampler->gradientFilter = gradientFilter;
}

export void EXPORT_UNIQUE(Sampler_destroy,
                          void *uniform _sampler)
{
  Sampler *uniform sampler = (Sampler *uniform)_sampler;
  delete sampler;
}

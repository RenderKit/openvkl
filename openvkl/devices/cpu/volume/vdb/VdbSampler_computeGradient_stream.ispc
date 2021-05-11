// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/export_util.h"

#include "VdbGrid.h"
#include "VdbSampler.ih"
#include "VdbSampler_filter.ih"

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

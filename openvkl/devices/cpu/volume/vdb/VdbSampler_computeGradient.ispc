// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/export_util.h"

#include "VdbGrid.h"
#include "VdbSampler.ih"
#include "VdbSampler_filter.ih"

export void EXPORT_UNIQUE(VdbSampler_computeGradient,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
                          const uniform uint32 attributeIndex,
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
    const varying float *uniform time = (const varying float *uniform)_time;

    vec3f gradient;
    __vkl_switch_filter(sampler->super.gradientFilter,
                        gradient = VdbSampler_computeGradient,
                        sampler,
                        indexCoordinates,
                        *time,
                        attributeIndex);

    // Note: xfmNormal takes inverse!
    *gradients = xfmNormal(sampler->grid->objectToIndex, gradient);
  }
}

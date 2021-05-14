// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "common/export_util.h"

#include "VdbGrid.h"
#include "VdbSampler.ih"
#include "VdbSampler_filter.ih"

export void EXPORT_UNIQUE(VdbSampler_computeSampleM,
                          const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const float *uniform _time,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          float *uniform samples)
{
  if (imask[programIndex]) {
    const VdbSampler *uniform sampler = (const VdbSampler *uniform)_sampler;
    assert(sampler);
    assert(sampler->grid);

    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    const varying float *uniform time = (const varying float *uniform)_time;

    const vec3f indexCoordinates =
        xfmPoint(sampler->grid->objectToIndex, *objectCoordinates);

    __vkl_switch_filter(sampler->super.filter,
                        VdbSampler_interpolate,
                        sampler,
                        indexCoordinates,
                        *time,
                        M,
                        attributeIndices,
                        samples);
  }
}

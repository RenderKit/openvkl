// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.ih"

#include "../../cpu/sampler/SamplerShared.h"
#include "../../cpu/volume/vdb/VdbGrid.h"
#include "../../cpu/volume/vdb/VdbSamplerShared.h"

#include "common.h"

namespace ispc {

#include "vdb/VdbSampler_filter.h"

  inline float VdbSampler_computeSample_uniform(
      const SamplerShared *samplerShared,
      const vec3f &objectCoordinates,
      const float &time,
      const uint32 &attributeIndex)
  {
    assert(sampler);
    assert(sampler->grid);

    const VdbSamplerShared *sampler =
        reinterpret_cast<const VdbSamplerShared *>(samplerShared);

    const vec3f indexCoordinates = openvkl::cpu_device::xfmPoint(
        sampler->grid->objectToIndex, objectCoordinates);

    float sample = 0.f;

    __vkl_switch_filter(sampler->super.super.filter,
                        sample = VdbSampler_interpolate,
                        sampler,
                        indexCoordinates,
                        time,
                        attributeIndex);

    return sample;
  }

  inline void VdbSampler_computeSampleM_uniform(const VdbSamplerShared *sampler,
                                                const vec3f &objectCoordinates,
                                                const float &time,
                                                const uint32 M,
                                                const uint32 *attributeIndices,
                                                float *samples)
  {
    assert(sampler);
    assert(sampler->grid);

    const vec3f indexCoordinates = openvkl::cpu_device::xfmPoint(
        sampler->grid->objectToIndex, objectCoordinates);

    __vkl_switch_filter(sampler->super.super.filter,
                        VdbSampler_interpolate,
                        sampler,
                        indexCoordinates,
                        time,
                        M,
                        attributeIndices,
                        samples);
  }

  inline vkl_vec3f VdbSampler_computeGradient_uniform(
      const VdbSamplerShared *sampler,
      const vec3f &objectCoordinates,
      const float &time,
      const uint32 &attributeIndex)
  {
    assert(sampler);
    assert(sampler->grid);

    const vec3f indexCoordinates = openvkl::cpu_device::xfmPoint(
        sampler->grid->objectToIndex, objectCoordinates);

    vec3f gradient = 0.f;

    __vkl_switch_filter(sampler->super.super.gradientFilter,
                        gradient = VdbSampler_computeGradient,
                        sampler,
                        indexCoordinates,
                        time,
                        attributeIndex);

    // Note: xfmNormal takes inverse!
    gradient =
        openvkl::cpu_device::xfmNormal(sampler->grid->objectToIndex, gradient);

    return vkl_vec3f{gradient.x, gradient.y, gradient.z};
  }

}  // namespace ispc

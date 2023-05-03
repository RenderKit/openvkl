// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../cpu/volume/amr/AMRVolumeShared.h"

#include "amr/CellRef.h"
#include "amr/DualCell.h"

namespace ispc {

  ////////////////////////////////////////////////////////////////////////////
  // `current` sampling method ///////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline void AMRVolume_transformObjectToLocal(const AMRVolume *volume,
                                               const vec3f &objectCoordinates,
                                               vec3f &localCoordinates)
  {
    localCoordinates =
        rcp(volume->gridSpacing) * (objectCoordinates - volume->gridOrigin);
  }

  inline float AMR_current(const SamplerShared *self, const vec3f &P)
  {
    const AMRVolume *volume = (const AMRVolume *)self->volume;
    const AMR *amr          = &volume->amr;

    if (!box_contains(volume->boundingBox, P)) {
      return volume->super.super.background[0];
    }

    vec3f lP;  // local amr space
    AMRVolume_transformObjectToLocal(volume, P, lP);

    const CellRef C = findLeafCell(amr, lP);

    DualCell D;
    initDualCell(D, lP, C.width);
    findDualCell(amr, D);

    return lerp(D);
  }

  ////////////////////////////////////////////////////////////////////////////
  // API entrypoints /////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline float AMRVolume_sample(const SamplerShared *sampler,
                                const vec3f &objectCoordinates,
                                const float &_time,
                                const uint32_t &_attributeIndex,
                                const VKLFeatureFlags _featureFlags)
  {
    return AMR_current(sampler, objectCoordinates);
  }

  inline void AMRVolume_sampleM(const SamplerShared *sampler,
                                const vec3f &objectCoordinates,
                                const uint32_t M,
                                const uint32_t *attributeIndices,
                                float *samples,
                                const VKLFeatureFlags _featureFlags)
  {
    for (uint32_t i = 0; i < M; i++) {
      // we still support this API, but AMR volumes only support a
      // single attribute
      assert(attributeIndices[i] == 0);

      samples[i] =
          AMRVolume_sample(sampler, objectCoordinates, 0.f, 0, _featureFlags);
    }
  }

  inline vkl_vec3f AMRVolume_computeGradient(
      const SamplerShared *sampler,
      const vec3f &objectCoordinates,
      const VKLFeatureFlags _featureFlags)
  {
    const AMRVolume *volume = (const AMRVolume *)sampler->volume;

    // gradient step in each dimension (object coordinates)
    const vec3f gradientStep = make_vec3f(volume->samplingStep * .1f);

    // compute via forward differences
    vec3f gradient;

    float sample =
        AMRVolume_sample(sampler, objectCoordinates, 0.f, 0, _featureFlags);

    gradient.x =
        AMRVolume_sample(sampler,
                         objectCoordinates + vec3f(gradientStep.x, 0.f, 0.f),
                         0.f,
                         0,
                         _featureFlags) -
        sample;
    gradient.y =
        AMRVolume_sample(sampler,
                         objectCoordinates + vec3f(0.f, gradientStep.y, 0.f),
                         0.f,
                         0,
                         _featureFlags) -
        sample;
    gradient.z =
        AMRVolume_sample(sampler,
                         objectCoordinates + vec3f(0.f, 0.f, gradientStep.z),
                         0.f,
                         0,
                         _featureFlags) -
        sample;

    return vkl_vec3f{gradient.x / gradientStep.x,
                     gradient.y / gradientStep.y,
                     gradient.z / gradientStep.z};
  }
}  // namespace ispc

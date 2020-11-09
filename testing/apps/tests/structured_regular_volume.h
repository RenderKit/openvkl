// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

template <typename PROCEDURAL_VOLUME_TYPE>
inline void sampling_on_vertices_vs_procedural_values(
    vec3i dimensions,
    VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
    size_t byteStride                      = 0,
    vec3i step                             = vec3i(1))
{
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f);

  auto v = rkcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         TemporalConfig(),
                                                         dataCreationFlags,
                                                         byteStride);

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    const float proceduralValue = v->computeProceduralValue(objectCoordinates);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    test_scalar_and_vector_sampling(
        vklSampler, objectCoordinates, proceduralValue, 1e-4f);
  }

  vklRelease(vklSampler);
}

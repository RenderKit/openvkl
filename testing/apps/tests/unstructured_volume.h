// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

inline void scalar_sampling_on_vertices_vs_procedural_values(
    vec3i dimensions,
    VKLUnstructuredCellType primType,
    VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
    size_t byteStride                      = 0,
    vec3i step                             = vec3i(1))
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          dimensions,
          vec3f(0.f),
          vec3f(1.f),
          primType,
          true,  // matching defaults of ProceduralUnstructuredVolume...
          true,
          false,
          false,
          dataCreationFlags,
          byteStride));

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    vec3f objectCoordinates =
        v->getGridOrigin() + offsetWithStep * v->getGridSpacing();

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    vec3f offsetCoordinates = objectCoordinates + vec3f(0.1f);
    CHECK(
        vklComputeSample(&vklSampler, (const vkl_vec3f *)&(offsetCoordinates)) ==
        Approx(v->computeProceduralValue(objectCoordinates)).margin(1e-4f));
  }

  vklRelease2(vklSampler);
}

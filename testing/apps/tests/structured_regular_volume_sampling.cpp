// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace ospcommon;
using namespace openvkl::testing;

template <typename PROCEDURAL_VOLUME_TYPE>
void sampling_on_vertices_vs_procedural_values(vec3i dimensions,
                                               vec3i step = vec3i(1))
{
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f);

  auto v = ospcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume = v->getVKLVolume();
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

TEST_CASE("Structured regular volume sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("32-bit addressing")
  {
    SECTION("unsigned char")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeUChar>(vec3i(128));
    }

    SECTION("short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeShort>(vec3i(128));
    }

    SECTION("unsigned short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeUShort>(vec3i(128));
    }

    SECTION("float")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeFloat>(vec3i(128));
    }

    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeDouble>(vec3i(128));
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64/32-bit addressing")
  {
    SECTION("unsigned char")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeUChar>(vec3i(1025), 16);
    }

    SECTION("short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeShort>(vec3i(813), 16);
    }

    SECTION("unsigned short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeUShort>(vec3i(813), 16);
    }

    SECTION("float")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeFloat>(vec3i(646), 16);
    }

    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeDouble>(vec3i(513), 16);
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64-bit addressing")
  {
    // only do double tests here for now; larger grids incur even more grid
    // accelerator build overhead, which we need to resolve.
    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeDouble>(vec3i(11586, 11586, 2),
                                                vec3i(16, 16, 1));
    }
  }
}

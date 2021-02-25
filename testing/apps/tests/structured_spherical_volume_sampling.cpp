// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

// does NOT test at boundary vertices
template <typename PROCEDURAL_VOLUME_TYPE>
void sampling_on_interior_vertices_vs_procedural_values(vec3i dimensions,
                                                        vec3i step = vec3i(1))
{
  const float boundingBoxSize = 1.f;

  vec3f gridOrigin;
  vec3f gridSpacing;

  // generate legal grid parameters
  PROCEDURAL_VOLUME_TYPE::generateGridParameters(
      dimensions, boundingBoxSize, gridOrigin, gridSpacing);

  auto v = rkcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    // skip boundary vertices
    if (reduce_min(offsetWithStep) == 0 ||
        offsetWithStep.x == dimensions.x - 1 ||
        offsetWithStep.y == dimensions.y - 1 ||
        offsetWithStep.z == dimensions.z - 1) {
      continue;
    }

    vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    const float proceduralValue = v->computeProceduralValue(objectCoordinates);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    test_scalar_and_vector_sampling(
        vklSampler, objectCoordinates, proceduralValue, 1e-3f);
  }

  vklRelease(vklSampler);
}

TEST_CASE("Structured spherical volume sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("32-bit addressing")
  {
    SECTION("unsigned char")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUChar>(vec3i(128));
    }

    SECTION("short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeShort>(vec3i(128));
    }

    SECTION("unsigned short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUShort>(vec3i(128));
    }

    SECTION("half")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeHalf>(vec3i(128));
    }

    SECTION("float")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeFloat>(vec3i(128));
    }

    SECTION("double")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeDouble>(vec3i(128));
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64/32-bit addressing")
  {
    SECTION("unsigned char")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUChar>(vec3i(1025), 16);
    }

    SECTION("short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeShort>(vec3i(813), 16);
    }

    SECTION("unsigned short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUShort>(vec3i(813), 16);
    }

    SECTION("half")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeHalf>(vec3i(813), 16);
    }

    SECTION("float")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeFloat>(vec3i(646), 16);
    }

    SECTION("double")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeDouble>(vec3i(513), 16);
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64-bit addressing, double")
  {
    // only do double tests here for now; larger grids incur even more grid
    // accelerator build overhead, which we need to resolve.
    sampling_on_interior_vertices_vs_procedural_values<
        WaveletStructuredSphericalVolumeDouble>(vec3i(11586, 11586, 2),
                                                vec3i(16, 16, 1));
  }
}

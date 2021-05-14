// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"
#include "structured_regular_volume.h"

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

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
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
  initializeOpenVKL();

  SECTION("32-bit addressing")
  {
    const vec3i dimensions = vec3i(128);
    const vec3i step       = vec3i(2);

    SECTION("unsigned char")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUChar>(dimensions, step);
    }

    SECTION("short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeShort>(dimensions, step);
    }

    SECTION("unsigned short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUShort>(dimensions, step);
    }

    SECTION("half")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeHalf>(dimensions, step);
    }

    SECTION("float")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeFloat>(dimensions, step);
    }

    SECTION("double")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeDouble>(dimensions, step);
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64/32-bit addressing")
  {
    const vec3i step = vec3i(32);

    SECTION("unsigned char")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUChar>(
          get_dimensions_for_64_32bit_addressing(sizeof(unsigned char)), step);
    }

    SECTION("short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeShort>(
          get_dimensions_for_64_32bit_addressing(sizeof(short)), step);
    }

    SECTION("unsigned short")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeUShort>(
          get_dimensions_for_64_32bit_addressing(sizeof(unsigned short)), step);
    }

    SECTION("half")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeHalf>(
          get_dimensions_for_64_32bit_addressing(sizeof(half_float::half)),
          step);
    }

    SECTION("float")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeFloat>(
          get_dimensions_for_64_32bit_addressing(sizeof(float)), step);
    }

    SECTION("double")
    {
      sampling_on_interior_vertices_vs_procedural_values<
          WaveletStructuredSphericalVolumeDouble>(
          get_dimensions_for_64_32bit_addressing(sizeof(double)), step);
    }
  }

  // 64-bit addressing mode is sufficiently addressed in structured regular
  // sampling tests

  shutdownOpenVKL();
}

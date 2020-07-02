// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "structured_regular_volume.h"

using namespace rkcommon;
using namespace openvkl::testing;

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
          WaveletStructuredRegularVolumeUChar>(
          vec3i(1025), VKL_DATA_DEFAULT, 0, 16);
    }

    SECTION("short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeShort>(
          vec3i(813), VKL_DATA_DEFAULT, 0, 16);
    }

    SECTION("unsigned short")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeUShort>(
          vec3i(813), VKL_DATA_DEFAULT, 0, 16);
    }

    SECTION("float")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeFloat>(
          vec3i(646), VKL_DATA_DEFAULT, 0, 16);
    }

    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeDouble>(
          vec3i(513), VKL_DATA_DEFAULT, 0, 16);
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
          WaveletStructuredRegularVolumeDouble>(
          vec3i(11586, 11586, 2), VKL_DATA_DEFAULT, 0, vec3i(16, 16, 1));
    }
  }
}

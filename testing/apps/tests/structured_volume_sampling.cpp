// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

template <typename VOXEL_TYPE>
void scalar_sampling_on_vertices_vs_procedural_values(vec3i dimensions,
                                                      vec3i step = vec3i(1))
{
  std::unique_ptr<
      ProceduralStructuredVolume<VOXEL_TYPE, getWaveletValue<VOXEL_TYPE>>>
      v(new ProceduralStructuredVolume<VOXEL_TYPE, getWaveletValue<VOXEL_TYPE>>(
          dimensions, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

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
    REQUIRE(
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates) ==
        Approx(v->computeProceduralValue(objectCoordinates)).margin(1e-4f));
  }
}

TEST_CASE("Structured volume sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("32-bit addressing")
  {
    SECTION("unsigned char")
    {
      scalar_sampling_on_vertices_vs_procedural_values<unsigned char>(
          vec3i(128));
    }

    SECTION("short")
    {
      scalar_sampling_on_vertices_vs_procedural_values<short>(vec3i(128));
    }

    SECTION("unsigned short")
    {
      scalar_sampling_on_vertices_vs_procedural_values<unsigned short>(
          vec3i(128));
    }

    SECTION("float")
    {
      scalar_sampling_on_vertices_vs_procedural_values<float>(vec3i(128));
    }

    SECTION("double")
    {
      scalar_sampling_on_vertices_vs_procedural_values<double>(vec3i(128));
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64/32-bit addressing")
  {
    SECTION("unsigned char")
    {
      scalar_sampling_on_vertices_vs_procedural_values<unsigned char>(
          vec3i(1025), 16);
    }

    SECTION("short")
    {
      scalar_sampling_on_vertices_vs_procedural_values<short>(vec3i(813), 16);
    }

    SECTION("unsigned short")
    {
      scalar_sampling_on_vertices_vs_procedural_values<unsigned short>(
          vec3i(813), 16);
    }

    SECTION("float")
    {
      scalar_sampling_on_vertices_vs_procedural_values<float>(vec3i(646), 16);
    }

    SECTION("double")
    {
      scalar_sampling_on_vertices_vs_procedural_values<double>(vec3i(513), 16);
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
      scalar_sampling_on_vertices_vs_procedural_values<double>(
          vec3i(11586, 11586, 2), vec3i(16, 16, 1));
    }
  }
}

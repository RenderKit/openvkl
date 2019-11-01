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
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

template <typename VOXEL_TYPE>
void sampling_on_vertices_vs_procedural_values(vec3i dimensions,
                                               vec3i step = vec3i(1))
{
  std::unique_ptr<
      ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                        getWaveletValue<VOXEL_TYPE>>>
      v(new ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                              getWaveletValue<VOXEL_TYPE>>(
          dimensions, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    vec3f objectCoordinates =
        v->getGridOrigin() + offsetWithStep * v->getGridSpacing();

    const float proceduralValue = v->computeProceduralValue(objectCoordinates);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    float scalarSampledValue =
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates);

    REQUIRE(scalarSampledValue == Approx(proceduralValue).margin(1e-4f));

    // since vklComputeSample() can have a specialized implementation separate
    // from vector sampling, check the vector APIs as well. we only need to
    // check for consistency with the scalar API result, as that has already
    // been validated.

    // first lane active only
    std::vector<int> valid(16, 0);
    valid[0] = 1;

    std::vector<vec3f> objectCoordinatesVector;
    objectCoordinatesVector.push_back(objectCoordinates);

    std::vector<float> objectCoordinatesSOA;

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 4);
    float samples_4[4]   = {0.f};
    vklComputeSample4(valid.data(),
                      vklVolume,
                      (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                      samples_4);

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
    float samples_8[8]   = {0.f};
    vklComputeSample8(valid.data(),
                      vklVolume,
                      (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                      samples_8);

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
    float samples_16[16] = {0.f};
    vklComputeSample16(valid.data(),
                       vklVolume,
                       (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                       samples_16);

    REQUIRE(scalarSampledValue == samples_4[0]);
    REQUIRE(scalarSampledValue == samples_8[0]);
    REQUIRE(scalarSampledValue == samples_16[0]);
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
      sampling_on_vertices_vs_procedural_values<unsigned char>(vec3i(128));
    }

    SECTION("short")
    {
      sampling_on_vertices_vs_procedural_values<short>(vec3i(128));
    }

    SECTION("unsigned short")
    {
      sampling_on_vertices_vs_procedural_values<unsigned short>(vec3i(128));
    }

    SECTION("float")
    {
      sampling_on_vertices_vs_procedural_values<float>(vec3i(128));
    }

    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<double>(vec3i(128));
    }
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64/32-bit addressing")
  {
    SECTION("unsigned char")
    {
      sampling_on_vertices_vs_procedural_values<unsigned char>(vec3i(1025), 16);
    }

    SECTION("short")
    {
      sampling_on_vertices_vs_procedural_values<short>(vec3i(813), 16);
    }

    SECTION("unsigned short")
    {
      sampling_on_vertices_vs_procedural_values<unsigned short>(vec3i(813), 16);
    }

    SECTION("float")
    {
      sampling_on_vertices_vs_procedural_values<float>(vec3i(646), 16);
    }

    SECTION("double")
    {
      sampling_on_vertices_vs_procedural_values<double>(vec3i(513), 16);
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
      sampling_on_vertices_vs_procedural_values<double>(vec3i(11586, 11586, 2),
                                                        vec3i(16, 16, 1));
    }
  }
}

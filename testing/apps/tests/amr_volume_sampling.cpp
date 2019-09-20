// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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
void amr_sampling_at_shell_boundaries(vec3i dimensions, vec3i step = vec3i(1))
{
  std::unique_ptr<ProceduralAMRVolume<VOXEL_TYPE, getShellValue>> v(
      new ProceduralAMRVolume<VOXEL_TYPE, getShellValue>(
          dimensions, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  std::vector<vec3f> offsets;
  offsets.emplace_back(0.0f, 0.0f, 0.0f);
  offsets.emplace_back(0.5f, 0.5f, 0.5f);
  offsets.emplace_back(1.0f, 1.0f, 1.0f);
  offsets.emplace_back(28.5f, 28.5f, 28.5f);
  offsets.emplace_back(29.5f, 29.5f, 29.5f);
  offsets.emplace_back(42.5f, 42.5f, 42.5f);
  offsets.emplace_back(43.5f, 43.5f, 43.5f);
  offsets.emplace_back(84.5f, 84.5f, 84.5f);
  offsets.emplace_back(85.5f, 85.5f, 85.5f);

  for (const vec3f &offset : offsets) {
    const auto offsetWithStep = offset * step;

    vec3f objectCoordinates = v->gridOrigin + offsetWithStep * v->gridSpacing;
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

TEST_CASE("AMR volume sampling", "[amr_volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("32-bit addressing")
  {
    SECTION("float")
    {
      amr_sampling_at_shell_boundaries<float>(vec3i(256));
    }
  }
}

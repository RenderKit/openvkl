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

void amr_sampling_at_shell_boundaries(vec3i dimensions, vec3i step = vec3i(1))
{
  std::unique_ptr<ProceduralShellsAMRVolume<>> v(
      new ProceduralShellsAMRVolume<>(
          dimensions, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  std::vector<vec3f> offsets;
  offsets.emplace_back(0.0f);
  offsets.emplace_back(0.5f);
  offsets.emplace_back(1.0f);

  offsets.emplace_back(63.0f);
  offsets.emplace_back(63.5f);
  // interpolation in range [64, 66)
  offsets.emplace_back(66.0f);

  offsets.emplace_back(96.0f);
  offsets.emplace_back(97.0f);

  offsets.emplace_back(111.0f);
  // interpolation at [112]
  offsets.emplace_back(114.0f);
  offsets.emplace_back(128.0f);

  for (const vec3f &offset : offsets) {
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

TEST_CASE("AMR volume sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("32-bit addressing")
  {
    amr_sampling_at_shell_boundaries(vec3i(256));
  }
}

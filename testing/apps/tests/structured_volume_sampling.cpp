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
#include "ospray/ospcommon/multidim_index_sequence.h"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Structured volume sampling")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("scalar sampling on vertices vs procedural values")
  {
    multidim_index_sequence<3> mis(v->getDimensions());

    for (const auto &offset : mis) {
      vec3f objectCoordinates =
          v->getGridOrigin() + offset * v->getGridSpacing();

      INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);
      CHECK(
          vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates) ==
          Approx(v->computeProceduralValue(objectCoordinates)).margin(1e-4f));
    }
  }
}

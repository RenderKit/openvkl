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
#include "volley_testing.h"

using namespace ospcommon;
using namespace volley::testing;

TEST_CASE("Ray iterator intervals")
{
  vlyLoadModule("ispc_driver");

  VLYDriver driver = vlyNewDriver("ispc_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VLYVolume vlyVolume = v->getVLYVolume();

  SECTION("scalar interval continuity with no samples mask")
  {
    vly_vec3f origin{0.5f, 0.5f, -1.f};
    vly_vec3f direction{0.f, 0.f, 1.f};
    vly_range1f tRange{0.f, inf};

    VLYRayIterator rayIterator =
        vlyNewRayIterator(vlyVolume, &origin, &direction, &tRange, nullptr);

    VLYRayInterval rayIntervalPrevious, rayIntervalCurrent;

    for (int i = 0; vlyIterateInterval(rayIterator, &rayIntervalCurrent); i++) {
      INFO("rayInterval tRange = " << rayIntervalCurrent.tRange.lower << ", "
                                   << rayIntervalCurrent.tRange.upper);

      if (i == 0) {
        // first interval at expected beginning
        REQUIRE(rayIntervalCurrent.tRange.lower == 1.f);
      } else {
        // interval continuity
        REQUIRE(rayIntervalCurrent.tRange.lower ==
                rayIntervalPrevious.tRange.upper);
      }

      rayIntervalPrevious = rayIntervalCurrent;
    }

    // last interval at expected ending
    REQUIRE(rayIntervalPrevious.tRange.upper == 2.f);
  }
}

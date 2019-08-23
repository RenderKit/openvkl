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

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Interval iterator", "[interval_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("scalar interval continuity with no samples mask")
  {
    vkl_vec3f origin{0.5f, 0.5f, -1.f};
    vkl_vec3f direction{0.f, 0.f, 1.f};
    vkl_range1f tRange{0.f, inf};

    VKLIntervalIterator iterator;
    vklInitIntervalIterator(
        &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

    VKLInterval intervalPrevious, intervalCurrent;

    for (int i = 0; vklIterateInterval(&iterator, &intervalCurrent); i++) {
      INFO("interval tRange = " << intervalCurrent.tRange.lower << ", "
                                << intervalCurrent.tRange.upper);

      if (i == 0) {
        // first interval at expected beginning
        REQUIRE(intervalCurrent.tRange.lower == 1.f);
      } else {
        // interval continuity
        REQUIRE(intervalCurrent.tRange.lower == intervalPrevious.tRange.upper);
      }

      intervalPrevious = intervalCurrent;
    }

    // last interval at expected ending
    REQUIRE(intervalPrevious.tRange.upper == 2.f);
  }
}

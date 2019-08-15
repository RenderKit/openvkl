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

TEST_CASE("Ray iterator surfaces")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<ZProceduralVolume> v(
      new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("scalar surface iteration")
  {
    vkl_vec3f origin{0.5f, 0.5f, -1.f};
    vkl_vec3f direction{0.f, 0.f, 1.f};
    vkl_range1f tRange{0.f, inf};

    VKLSamplesMask samplesMask = vklNewSamplesMask(vklVolume);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    vklSamplesMaskSetValues(samplesMask, isoValues.size(), isoValues.data());

    vklCommit((VKLObject)samplesMask);

    VKLRayIterator rayIterator;
    vklInitRayIterator(
        &rayIterator, vklVolume, &origin, &direction, &tRange, samplesMask);

    VKLHit hit;

    int hitCount = 0;

    while (vklIterateSurface(&rayIterator, &hit)) {
      INFO("hit t = " << hit.t << ", sample = " << hit.sample);

      REQUIRE(hit.t == 1.f + isoValues[hitCount]);
      REQUIRE(hit.sample == isoValues[hitCount]);

      hitCount++;
    }

    REQUIRE(hitCount == isoValues.size());
  }
}

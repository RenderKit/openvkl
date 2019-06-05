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

TEST_CASE("Ray iterator surfaces")
{
  vlyLoadModule("ispc_driver");

  VLYDriver driver = vlyNewDriver("ispc_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<ZProceduralVolume> v(
      new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VLYVolume vlyVolume = v->getVLYVolume();

  SECTION("scalar surface iteration")
  {
    vly_vec3f origin{0.5f, 0.5f, -1.f};
    vly_vec3f direction{0.f, 0.f, 1.f};
    vly_range1f tRange{0.f, inf};

    VLYSamplesMask samplesMask = vlyNewSamplesMask(vlyVolume);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    vlySamplesMaskSetValues(samplesMask, isoValues.size(), isoValues.data());

    vlyCommit((VLYObject)samplesMask);

    VLYRayIterator rayIterator =
        vlyNewRayIterator(vlyVolume, &origin, &direction, &tRange, samplesMask);

    VLYSurfaceHit surfaceHit;

    int hitCount = 0;

    while (vlyIterateSurface(&rayIterator, &surfaceHit)) {
      INFO("surfaceHit t = " << surfaceHit.t
                             << ", sample = " << surfaceHit.sample);

      REQUIRE(surfaceHit.t == 1.f + isoValues[hitCount]);
      REQUIRE(surfaceHit.sample == isoValues[hitCount]);

      hitCount++;
    }

    REQUIRE(hitCount == isoValues.size());
  }
}

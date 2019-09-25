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

void scalar_hit_iteration(VKLVolume volume, const std::vector<float> &isoValues)
{
  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  VKLValueSelector valueSelector = vklNewValueSelector(volume);

  vklValueSelectorSetValues(valueSelector, isoValues.size(), isoValues.data());

  vklCommit(valueSelector);

  VKLHitIterator iterator;
  vklInitHitIterator(
      &iterator, volume, &origin, &direction, &tRange, valueSelector);

  VKLHit hit;

  int hitCount = 0;

  while (vklIterateHit(&iterator, &hit)) {
    INFO("hit t = " << hit.t << ", sample = " << hit.sample);

    REQUIRE(hit.t == 1.f + isoValues[hitCount]);
    REQUIRE(hit.sample == isoValues[hitCount]);

    hitCount++;
  }

  REQUIRE(hitCount == isoValues.size());
}

TEST_CASE("Hit iterator", "[hit_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  // default isovalues
  std::vector<float> defaultIsoValues;

  for (float f = 0.1f; f < 1.f; f += 0.1f) {
    defaultIsoValues.push_back(f);
  }

  SECTION("scalar hit iteration")
  {
    SECTION("structured volumes")
    {
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

      VKLVolume vklVolume = v->getVKLVolume();

      scalar_hit_iteration(vklVolume, defaultIsoValues);
    }

    SECTION(
        "structured volumes: isovalues at grid accelerator macrocell "
        "boundaries")
    {
      // macrocells are currently 16**3
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

      VKLVolume vklVolume = v->getVKLVolume();

      std::vector<float> macroCellBoundaries;

      for (int i = 0; i < 128; i += 16) {
        macroCellBoundaries.push_back(float(i));
      }

      scalar_hit_iteration(vklVolume, macroCellBoundaries);
    }

    SECTION("unstructured volumes")
    {
      std::unique_ptr<ZUnstructuredProceduralVolume> v(
          new ZUnstructuredProceduralVolume(
              dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));

      VKLVolume vklVolume = v->getVKLVolume();

      scalar_hit_iteration(vklVolume, defaultIsoValues);
    }
  }
}

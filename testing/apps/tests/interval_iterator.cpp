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

vkl_range1f computeIntervalValueRange(VKLVolume volume,
                                      const vkl_vec3f &origin,
                                      const vkl_vec3f &direction,
                                      const vkl_range1f &tRange)
{
  vkl_range1f sampledValueRange{inf, neg_inf};

  constexpr int numValueRangeSamples = 100;

  for (int i = 0; i < numValueRangeSamples; i++) {
    float t = tRange.lower + float(i) / float(numValueRangeSamples - 1) *
                                 (tRange.upper - tRange.lower);

    vkl_vec3f c{origin.x + t * direction.x,
                origin.y + t * direction.y,
                origin.z + t * direction.z};

    float sample = vklComputeSample(volume, &c);

    sampledValueRange.lower = std::min(sampledValueRange.lower, sample);
    sampledValueRange.upper = std::max(sampledValueRange.upper, sample);
  }

  return sampledValueRange;
}

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

  SECTION("scalar interval value ranges with no samples mask")
  {
    vkl_vec3f origin{0.5f, 0.5f, -1.f};
    vkl_vec3f direction{0.f, 0.f, 1.f};
    vkl_range1f tRange{0.f, inf};

    VKLIntervalIterator iterator;
    vklInitIntervalIterator(
        &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

    VKLInterval interval;

    int intervalCount = 0;

    while (vklIterateInterval(&iterator, &interval)) {
      INFO("interval tRange = " << interval.tRange.lower << ", "
                                << interval.tRange.upper
                                << " valueRange = " << interval.valueRange.lower
                                << ", " << interval.valueRange.upper);

      vkl_range1f sampledValueRange = computeIntervalValueRange(
          vklVolume, origin, direction, interval.tRange);

      INFO("sampled value range = " << sampledValueRange.lower << ", "
                                    << sampledValueRange.upper);

      // the sampled value range should be completely within the returned
      // interval value range
      REQUIRE(sampledValueRange.lower >= interval.valueRange.lower);
      REQUIRE(sampledValueRange.upper <= interval.valueRange.upper);

      float rangeOverlapFraction =
          (sampledValueRange.upper - sampledValueRange.lower) /
          (interval.valueRange.upper - interval.valueRange.lower);

      // warn if we have overly conservative returned interval value range
      if (rangeOverlapFraction < 0.25f) {
        WARN("sampled value range is less than "
             << rangeOverlapFraction << "x the returned interval value range");
      }

      intervalCount++;
    }

    // make sure we had at least one interval...
    REQUIRE(intervalCount > 0);
  }
}

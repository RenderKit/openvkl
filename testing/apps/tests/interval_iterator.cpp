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
#include "iterator_utility.h"
#include "openvkl_testing.h"
#include "ospcommon/math/box.h"

using namespace ospcommon;
using namespace openvkl::testing;

void scalar_interval_continuity_with_no_value_selector(VKLVolume volume)
{
  const vkl_box3f vklBoundingBox = vklGetBoundingBox(volume);
  const box3f boundingBox        = (const box3f &)vklBoundingBox;

  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  range1f expectedTRange = intersectRayBox(
      (const vec3f &)origin, (const vec3f &)direction, boundingBox);

  VKLIntervalIterator iterator;
  vklInitIntervalIterator(
      &iterator, volume, &origin, &direction, &tRange, nullptr);

  VKLInterval intervalPrevious, intervalCurrent;

  for (int i = 0; vklIterateInterval(&iterator, &intervalCurrent); i++) {
    INFO("interval tRange = " << intervalCurrent.tRange.lower << ", "
                              << intervalCurrent.tRange.upper);

    if (i == 0) {
      // first interval at expected beginning
      REQUIRE(intervalCurrent.tRange.lower == Approx(expectedTRange.lower));
    } else {
      // interval continuity
      REQUIRE(intervalCurrent.tRange.lower == intervalPrevious.tRange.upper);
    }

    intervalPrevious = intervalCurrent;
  }

  // last interval at expected ending
  REQUIRE(intervalPrevious.tRange.upper == Approx(expectedTRange.upper));
}

void scalar_interval_value_ranges_with_no_value_selector(VKLVolume volume)
{
  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  VKLIntervalIterator iterator;
  vklInitIntervalIterator(
      &iterator, volume, &origin, &direction, &tRange, nullptr);

  VKLInterval interval;

  int intervalCount = 0;

  while (vklIterateInterval(&iterator, &interval)) {
    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange =
        computeIntervalValueRange(volume, origin, direction, interval.tRange);

    INFO("sampled value range = " << sampledValueRange.lower << ", "
                                  << sampledValueRange.upper);

    // the sampled value range should be completely within the returned
    // interval value range
    REQUIRE(sampledValueRange.lower >= interval.valueRange.lower);
    REQUIRE(sampledValueRange.upper <= interval.valueRange.upper);

    float rangeOverlapFraction =
        (sampledValueRange.upper - sampledValueRange.lower) /
        (interval.valueRange.upper - interval.valueRange.lower);

    // warn if we have overly conservative returned interval value range; note
    // this may trigger frequently for volumes without a native iterator
    // implementation.
    if (rangeOverlapFraction < 0.25f) {
      WARN("sampled value range is less than "
           << rangeOverlapFraction << "x the returned interval value range");
    }

    intervalCount++;
  }

  // make sure we had at least one interval...
  REQUIRE(intervalCount > 0);
}

void scalar_interval_value_ranges_with_value_selector(VKLVolume volume)
{
  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  VKLValueSelector valueSelector = vklNewValueSelector(volume);

  // will trigger intervals covering individual ranges separately
  std::vector<vkl_range1f> valueRanges{{0.9f, 1.f}, {1.9f, 2.f}};

  vklValueSelectorSetRanges(
      valueSelector, valueRanges.size(), valueRanges.data());

  vklCommit(valueSelector);

  VKLIntervalIterator iterator;
  vklInitIntervalIterator(
      &iterator, volume, &origin, &direction, &tRange, valueSelector);

  VKLInterval interval;

  int intervalCount = 0;

  while (vklIterateInterval(&iterator, &interval)) {
    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange =
        computeIntervalValueRange(volume, origin, direction, interval.tRange);

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

    // the interval value range should overlap the value selector value
    // range(s)
    bool rangeIntersectsValueSelector = false;

    for (const auto &r : valueRanges) {
      if (rangesIntersect(r, interval.valueRange)) {
        rangeIntersectsValueSelector = true;
        break;
      }
    }

    REQUIRE(rangeIntersectsValueSelector);

    intervalCount++;
  }

  // make sure we had at least one interval...
  REQUIRE(intervalCount > 0);

  vklRelease(valueSelector);
}

TEST_CASE("Interval iterator", "[interval_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("structured volumes")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

    std::unique_ptr<WaveletProceduralVolume> v(
        new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

    VKLVolume vklVolume = v->getVKLVolume();

    SECTION("scalar interval continuity with no value selector")
    {
      scalar_interval_continuity_with_no_value_selector(vklVolume);
    }

    SECTION("scalar interval value ranges with no value selector")
    {
      scalar_interval_value_ranges_with_no_value_selector(vklVolume);
    }

    SECTION("scalar interval value ranges with value selector")
    {
      scalar_interval_value_ranges_with_value_selector(vklVolume);
    }
  }

  SECTION("unstructured volumes")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

    std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
        new WaveletUnstructuredProceduralVolume(
            dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));

    VKLVolume vklVolume = v->getVKLVolume();

    SECTION("scalar interval continuity with no value selector")
    {
      scalar_interval_continuity_with_no_value_selector(vklVolume);
    }

    SECTION("scalar interval value ranges with no value selector")
    {
      scalar_interval_value_ranges_with_no_value_selector(vklVolume);
    }

    SECTION("scalar interval value ranges with value selector")
    {
      scalar_interval_value_ranges_with_value_selector(vklVolume);
    }
  }
}

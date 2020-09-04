// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "iterator_utility.h"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"

using namespace rkcommon;
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

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  std::vector<char> buffer(vklGetIntervalIteratorSize(sampler));
  VKLIntervalIterator iterator = vklInitIntervalIterator(
      sampler, &origin, &direction, &tRange, nullptr, buffer.data());

  VKLInterval intervalPrevious, intervalCurrent;

  for (int i = 0; vklIterateInterval(iterator, &intervalCurrent); i++) {
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
  vklRelease(sampler);
}

void scalar_interval_value_ranges_with_no_value_selector(VKLVolume volume)
{
  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  std::vector<char> buffer(vklGetIntervalIteratorSize(sampler));
  VKLIntervalIterator iterator = vklInitIntervalIterator(
      sampler, &origin, &direction, &tRange, nullptr, buffer.data());
  VKLInterval interval;

  int intervalCount = 0;

  while (vklIterateInterval(iterator, &interval)) {
    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange =
        computeIntervalValueRange(sampler, origin, direction, interval.tRange);

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

  vklRelease(sampler);

  // make sure we had at least one interval...
  REQUIRE(intervalCount > 0);
}

void scalar_interval_value_ranges_with_value_selector(VKLVolume volume)
{
  vkl_vec3f origin{0.5f, 0.5f, -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, inf};

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLValueSelector valueSelector = vklNewValueSelector(volume);

  // will trigger intervals covering individual ranges separately
  std::vector<vkl_range1f> valueRanges{{0.9f, 1.f}, {1.9f, 2.f}};

  vklValueSelectorSetRanges(
      valueSelector, valueRanges.size(), valueRanges.data());

  vklCommit(valueSelector);

  std::vector<char> buffer(vklGetIntervalIteratorSize(sampler));
  VKLIntervalIterator iterator = vklInitIntervalIterator(
      sampler, &origin, &direction, &tRange, valueSelector, buffer.data());

  VKLInterval interval;

  int intervalCount = 0;

  while (vklIterateInterval(iterator, &interval)) {
    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange =
        computeIntervalValueRange(sampler, origin, direction, interval.tRange);

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
  vklRelease(sampler);
}

void scalar_interval_nominalDeltaT(VKLVolume volume,
                                   const vec3f &direction,
                                   const float expectedNominalDeltaT)
{
  // origin far away from the volume in the negative direction
  const float d = 1000.f;
  vkl_vec3f origin{-d * direction.x, -d * direction.y, -d * direction.z};

  vkl_range1f tRange{0.f, inf};

  INFO("direction = " << direction.x << " " << direction.y << " "
                      << direction.z);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  std::vector<char> buffer(vklGetIntervalIteratorSize(sampler));
  VKLIntervalIterator iterator =
      vklInitIntervalIterator(sampler,
                              &origin,
                              &(const vkl_vec3f &)direction,
                              &tRange,
                              nullptr,
                              buffer.data());

  VKLInterval interval;
  bool gotInterval = vklIterateInterval(iterator, &interval);

  REQUIRE(gotInterval == true);

  INFO("interval tRange = " << interval.tRange.lower << ", "
                            << interval.tRange.upper
                            << " valueRange = " << interval.valueRange.lower
                            << ", " << interval.valueRange.upper
                            << ", nominalDeltaT = " << interval.nominalDeltaT);

  REQUIRE(interval.nominalDeltaT == Approx(expectedNominalDeltaT));
  vklRelease(sampler);
}

TEST_CASE("Interval iterator", "[interval_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("structured volumes")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

    auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        dimensions, gridOrigin, gridSpacing);

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

  SECTION("structured volumes: interval nominalDeltaT")
  {
    // use a different volume to facilitate nominalDeltaT tests
    const vec3i dimensions(128);
    const vec3f gridOrigin(-64.f);
    const vec3f gridSpacing(1.f, 2.f, 3.f);

    auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        dimensions, gridOrigin, gridSpacing);

    VKLVolume vklVolume = v->getVKLVolume();

    SECTION("normalized directions")
    {
      SECTION("unit directions, positive and negative")
      {
        scalar_interval_nominalDeltaT(vklVolume, vec3f(1.f, 0.f, 0.f), 1.f);
        scalar_interval_nominalDeltaT(vklVolume, vec3f(-1.f, 0.f, 0.f), 1.f);

        scalar_interval_nominalDeltaT(vklVolume, vec3f(0.f, 1.f, 0.f), 2.f);
        scalar_interval_nominalDeltaT(vklVolume, vec3f(0.f, -1.f, 0.f), 2.f);

        scalar_interval_nominalDeltaT(vklVolume, vec3f(0.f, 0.f, 1.f), 3.f);
        scalar_interval_nominalDeltaT(vklVolume, vec3f(0.f, 0.f, -1.f), 3.f);
      }

      SECTION("diagonal direction, positive and negative")
      {
        scalar_interval_nominalDeltaT(
            vklVolume, normalize(gridSpacing), length(gridSpacing));

        scalar_interval_nominalDeltaT(
            vklVolume, -normalize(gridSpacing), length(gridSpacing));
      }
    }

    SECTION("non-normalized directions")
    {
      // nominalDeltaT should be in ray units, and thus scaled by the direction
      // magnitude

      const float scale = 10.f;

      SECTION("unit directions, positive and negative")
      {
        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(1.f, 0.f, 0.f), 1.f / scale);
        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(-1.f, 0.f, 0.f), 1.f / scale);

        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(0.f, 1.f, 0.f), 2.f / scale);
        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(0.f, -1.f, 0.f), 2.f / scale);

        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(0.f, 0.f, 1.f), 3.f / scale);
        scalar_interval_nominalDeltaT(
            vklVolume, scale * vec3f(0.f, 0.f, -1.f), 3.f / scale);
      }

      SECTION("diagonal direction, positive and negative")
      {
        scalar_interval_nominalDeltaT(vklVolume,
                                      scale * normalize(gridSpacing),
                                      length(gridSpacing) / scale);

        scalar_interval_nominalDeltaT(vklVolume,
                                      -scale * normalize(gridSpacing),
                                      length(gridSpacing) / scale);
      }
    }
  }

  SECTION("unstructured volumes")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

    auto v = rkcommon::make_unique<WaveletUnstructuredProceduralVolume>(
        dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);

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

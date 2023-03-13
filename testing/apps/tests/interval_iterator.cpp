// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "iterator_utility.h"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"
#include "wrappers.h"

using namespace rkcommon;
using namespace openvkl::testing;

inline void checkAndWarnValueRangeOverlap(const vkl_range1f &sampledValueRange,
                                          const VKLInterval &interval)
{
  float rangeOverlapFraction =
      (sampledValueRange.upper - sampledValueRange.lower) /
      (interval.valueRange.upper - interval.valueRange.lower);

  // warn if we have overly conservative returned interval value range; note
  // this may trigger frequently for volumes without a native iterator
  // implementation.
  //
  // note that some procedural volume attributes vary only in one direction. in
  // those cases the sampled value range can be zero depending on the ray
  // direction.
  if (rangeOverlapFraction < 0.25f) {
    WARN("sampled value range ["
         << sampledValueRange.lower << ", " << sampledValueRange.upper
         << "] is less than " << rangeOverlapFraction
         << "x the returned interval value range [" << interval.valueRange.lower
         << ", " << interval.valueRange.upper << "]");
  }
}

void scalar_interval_continuity_with_no_value_ranges(
    VKLVolume volume,
    const unsigned int attributeIndex,
    const vkl_vec3f &origin,
    const vkl_vec3f &direction)
{
  const vkl_box3f vklBoundingBox = vklGetBoundingBox(volume);
  const box3f boundingBox        = (const box3f &)vklBoundingBox;

  vkl_range1f tRange{0.f, inf};

  const float time = 0.f;

  range1f expectedTRange = intersectRayBox(
      (const vec3f &)origin, (const vec3f &)direction, boundingBox);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetInt(intervalContext, "attributeIndex", attributeIndex);

  vklCommit(intervalContext);

  VKLInterval *intervalsBuffer = allocate<VKLInterval>(maxNumIntervals);

  int *intervalCount = allocate<int>(1);

  // interval iteration
  char *iteratorBuffer =
      allocate<char>(vklGetIntervalIteratorSize(&intervalContext));

  auto testIteratorFunc = [=]() {
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIterator(&intervalContext,
                                &origin,
                                &direction,
                                &tRange,
                                time,
                                (void *)iteratorBuffer);
    VKLInterval intervalCurrent;
    while (vklIterateInterval(intervalIterator, &intervalCurrent)) {
      intervalsBuffer[*intervalCount] = intervalCurrent;
      *intervalCount += 1;
      if (*intervalCount >= maxNumIntervals) {
        break;
      }
    }
  };

  executeTestFunction(testIteratorFunc);

  if (*intervalCount >= maxNumIntervals) {
    WARN("Interval iterations reached max number of intervals: " << maxNumIntervals);
  }

  deallocate(iteratorBuffer);

  VKLInterval intervalPrevious;
  for (int i = 0; i < *intervalCount; i++) {
    VKLInterval intervalCurrent = intervalsBuffer[i];

    INFO("interval tRange = " << intervalCurrent.tRange.lower << ", "
                              << intervalCurrent.tRange.upper);

    if (i == 0) {
      // first interval at expected beginning
      REQUIRE(intervalCurrent.tRange.lower == Approx(expectedTRange.lower));
    } else {
      // interval continuity
      REQUIRE(intervalCurrent.tRange.lower == intervalPrevious.tRange.upper);
    }

    // all intervals should be contained in the bounding box tRange
    REQUIRE(intervalCurrent.tRange.lower >= Approx(expectedTRange.lower));
    REQUIRE(intervalCurrent.tRange.upper <= Approx(expectedTRange.upper));

    intervalPrevious = intervalCurrent;
  }

  // last interval at expected ending
  REQUIRE(intervalPrevious.tRange.upper == Approx(expectedTRange.upper));

  deallocate(intervalsBuffer);
  deallocate(intervalCount);

  vklRelease(intervalContext);
  vklRelease(sampler);
}

void scalar_interval_value_ranges_with_no_value_ranges(
    VKLVolume volume,
    const unsigned int attributeIndex,
    const vkl_vec3f &origin,
    const vkl_vec3f &direction)
{
  vkl_range1f tRange{0.f, inf};

  const float time = 0.f;

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetInt(intervalContext, "attributeIndex", attributeIndex);

  vklCommit(intervalContext);

  VKLInterval *intervalsBuffer         = allocate<VKLInterval>(maxNumIntervals);
  vkl_range1f *sampledValueRangeBuffer = allocate<vkl_range1f>(maxNumIntervals);

  int *intervalCount = allocate<int>(1);

  // interval iteration
  char *iteratorBuffer =
      allocate<char>(vklGetIntervalIteratorSize(&intervalContext));

  auto testIteratorFunc = [=]() {
    VKLIntervalIterator iterator =
        vklInitIntervalIterator(&intervalContext,
                                &origin,
                                &direction,
                                &tRange,
                                time,
                                (void *)iteratorBuffer);
    VKLInterval interval;
    while (vklIterateInterval(iterator, &interval)) {
      vkl_range1f sampledValueRange = computeIntervalValueRange(
          sampler, attributeIndex, origin, direction, interval.tRange);
      intervalsBuffer[*intervalCount]         = interval;
      sampledValueRangeBuffer[*intervalCount] = sampledValueRange;
      *intervalCount += 1;
      if (*intervalCount >= maxNumIntervals) {
        break;
      }
    }
  };

  executeTestFunction(testIteratorFunc);

  if (*intervalCount >= maxNumIntervals) {
    WARN("Interval iterations reached max number of intervals: " << maxNumIntervals);
  }

  deallocate(iteratorBuffer);

  for (int i = 0; i < *intervalCount; i++) {
    VKLInterval interval = intervalsBuffer[i];

    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange = sampledValueRangeBuffer[i];

    INFO("sampled value range = " << sampledValueRange.lower << ", "
                                  << sampledValueRange.upper);

    // the sampled value range should be completely within the returned
    // interval value range
    REQUIRE(Approx(sampledValueRange.lower) >= interval.valueRange.lower);
    REQUIRE(Approx(sampledValueRange.upper) <= interval.valueRange.upper);

    checkAndWarnValueRangeOverlap(sampledValueRange, interval);
  }

  deallocate(intervalsBuffer);
  deallocate(sampledValueRangeBuffer);

  vklRelease(intervalContext);
  vklRelease(sampler);

  // make sure we had at least one interval...
  REQUIRE(*intervalCount > 0);

  deallocate(intervalCount);
}

// each provided valueRange should occur on a different interval
void scalar_interval_value_ranges_with_value_ranges(
    VKLVolume volume,
    const unsigned int attributeIndex,
    const vkl_vec3f &origin,
    const vkl_vec3f &direction,
    const std::vector<vkl_range1f> valueRanges)
{
  vkl_range1f tRange{0.f, inf};

  const float time = 0.f;

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLData valueRangesData = vklNewData(
      getOpenVKLDevice(), valueRanges.size(), VKL_BOX1F, valueRanges.data());

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetInt(intervalContext, "attributeIndex", attributeIndex);

  vklSetData(intervalContext, "valueRanges", valueRangesData);
  vklRelease(valueRangesData);

  vklCommit(intervalContext);

  VKLInterval *intervalsBuffer         = allocate<VKLInterval>(maxNumIntervals);
  vkl_range1f *sampledValueRangeBuffer = allocate<vkl_range1f>(maxNumIntervals);

  int *intervalCount = allocate<int>(1);

  // interval iteration
  char *iteratorBuffer =
      allocate<char>(vklGetIntervalIteratorSize(&intervalContext));

  auto testIteratorFunc = [=]() {
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIterator(&intervalContext,
                                &origin,
                                &direction,
                                &tRange,
                                time,
                                (void *)iteratorBuffer);
    VKLInterval interval;
    while (vklIterateInterval(intervalIterator, &interval)) {
      intervalsBuffer[*intervalCount] = interval;
      vkl_range1f sampledValueRange   = computeIntervalValueRange(
          sampler, attributeIndex, origin, direction, interval.tRange);
      sampledValueRangeBuffer[*intervalCount] = sampledValueRange;
      *intervalCount += 1;
      if (*intervalCount >= maxNumIntervals) {
        break;
      }
    }
  };

  executeTestFunction(testIteratorFunc);

  if (*intervalCount >= maxNumIntervals) {
    WARN("Interval iterations reached max number of intervals: " << maxNumIntervals);
  }

  deallocate(iteratorBuffer);

  for (int i = 0; i < *intervalCount; i++) {
    VKLInterval interval = intervalsBuffer[i];
    INFO("interval tRange = " << interval.tRange.lower << ", "
                              << interval.tRange.upper
                              << " valueRange = " << interval.valueRange.lower
                              << ", " << interval.valueRange.upper);

    vkl_range1f sampledValueRange = sampledValueRangeBuffer[i];

    INFO("sampled value range = " << sampledValueRange.lower << ", "
                                  << sampledValueRange.upper);

    // the sampled value range should be completely within the returned
    // interval value range
    REQUIRE(Approx(sampledValueRange.lower) >= interval.valueRange.lower);
    REQUIRE(Approx(sampledValueRange.upper) <= interval.valueRange.upper);

    checkAndWarnValueRangeOverlap(sampledValueRange, interval);

    // the interval value range should overlap the value ranges value
    // range(s)
    bool rangeIntersectsContextValueRanges = false;

    for (const auto &r : valueRanges) {
      if (rangesIntersect(r, interval.valueRange)) {
        rangeIntersectsContextValueRanges = true;
        break;
      }
    }

    REQUIRE(rangeIntersectsContextValueRanges);
  }

  // make sure we had appropriate number of intervals
  REQUIRE(*intervalCount >= valueRanges.size());

  deallocate(intervalsBuffer);
  deallocate(sampledValueRangeBuffer);
  deallocate(intervalCount);

  vklRelease(intervalContext);
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

  const float time = 0.f;

  INFO("direction = " << direction.x << " " << direction.y << " "
                      << direction.z);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);
  vklCommit(intervalContext);

  char *iteratorBuffer =
      allocate<char>(vklGetIntervalIteratorSize(&intervalContext));

  bool *gotInterval     = allocate<bool>(1);
  VKLInterval *interval = allocate<VKLInterval>(1);

  auto testIteratorFunc = [=]() {
    VKLIntervalIterator intervalIterator =
        vklInitIntervalIterator(&intervalContext,
                                &origin,
                                &(const vkl_vec3f &)direction,
                                &tRange,
                                time,
                                (void *)iteratorBuffer);
    *gotInterval = vklIterateInterval(intervalIterator, interval);
  };

  executeTestFunction(testIteratorFunc);

  REQUIRE(*gotInterval == true);

  INFO("interval tRange = " << interval->tRange.lower << ", "
                            << interval->tRange.upper
                            << " valueRange = " << interval->valueRange.lower
                            << ", " << interval->valueRange.upper
                            << ", nominalDeltaT = " << interval->nominalDeltaT);

  REQUIRE(interval->nominalDeltaT == Approx(expectedNominalDeltaT));

  deallocate(iteratorBuffer);
  deallocate(gotInterval);
  deallocate(interval);

  vklRelease(intervalContext);
  vklRelease(sampler);
}

template <typename PROCEDURAL_VOLUME_TYPE>
void scalar_single_attribute_interval_iterator_tests(
    PROCEDURAL_VOLUME_TYPE &proceduralVolume)
{
  VKLVolume vklVolume = proceduralVolume->getVKLVolume(getOpenVKLDevice());

  const unsigned int attributeIndex = 0;

  const vkl_vec3f origin{0.5f, 0.5f, -1.f};
  const vkl_vec3f direction{0.f, 0.f, 1.f};

  // must trigger intervals covering individual ranges separately
  std::vector<vkl_range1f> valueRanges{{0.9f, 1.f}, {1.9f, 2.f}};

  scalar_interval_continuity_with_no_value_ranges(
      vklVolume, attributeIndex, origin, direction);

  scalar_interval_value_ranges_with_no_value_ranges(
      vklVolume, attributeIndex, origin, direction);

  scalar_interval_value_ranges_with_value_ranges(
      vklVolume, attributeIndex, origin, direction, valueRanges);
}

template <typename PROCEDURAL_VOLUME_TYPE>
void scalar_multi_attribute_interval_iterator_tests(
    PROCEDURAL_VOLUME_TYPE &proceduralVolume)
{
  VKLVolume vklVolume = proceduralVolume->getVKLVolume(getOpenVKLDevice());

  const unsigned int numAttributes = vklGetNumAttributes(vklVolume);

  // setup appropriate rays for the procedural volume
  assert(numAttributes == 4);
  std::vector<vkl_vec3f> origins{{0.5f, 0.5f, -1.f},
                                 {-1.f, 0.5f, 0.5f},
                                 {0.5f, -1.f, 0.5f},
                                 {0.5f, 0.5f, -1.f}};

  std::vector<vkl_vec3f> directions{
      {0.f, 0.f, 1.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};

  // must trigger intervals covering individual ranges separately
  std::vector<std::vector<vkl_range1f>> valueRanges;
  valueRanges.push_back(std::vector<vkl_range1f>{{0.9f, 1.f}, {1.9f, 2.f}});
  valueRanges.push_back(std::vector<vkl_range1f>{{0.1f, 0.2f}, {0.8f, 0.9f}});
  valueRanges.push_back(std::vector<vkl_range1f>{{0.1f, 0.2f}, {0.8f, 0.9f}});
  valueRanges.push_back(std::vector<vkl_range1f>{{0.1f, 0.2f}, {0.8f, 0.9f}});

  for (unsigned int i = 0; i < numAttributes; i++) {
    scalar_interval_continuity_with_no_value_ranges(
        vklVolume, i, origins[i], directions[i]);

    scalar_interval_value_ranges_with_no_value_ranges(
        vklVolume, i, origins[i], directions[i]);

    scalar_interval_value_ranges_with_value_ranges(
        vklVolume, i, origins[i], directions[i], valueRanges[i]);
  }
}

TEST_CASE("Interval iterator", "[interval_iterators]")
{
  initializeOpenVKL();

  SECTION("single attribute interval iteration")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
    SECTION("structured volumes")
    {
      auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
          dimensions, gridOrigin, gridSpacing);

      scalar_single_attribute_interval_iterator_tests(v);
    }
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED || defined(OPENVKL_TESTING_GPU)
    SECTION("unstructured volumes")
    {
      auto v = rkcommon::make_unique<WaveletUnstructuredProceduralVolume>(
          dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false);

      scalar_single_attribute_interval_iterator_tests(v);
    }
#endif

#if OPENVKL_DEVICE_CPU_VDB
    SECTION("VDB volumes")
    {
      for (const auto &repackNodes : {true, false}) {
        std::stringstream sectionName;
        sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

        DYNAMIC_SECTION(sectionName.str())
        {
          auto v =
              rkcommon::make_unique<WaveletVdbVolumeFloat>(getOpenVKLDevice(),
                                                           dimensions,
                                                           gridOrigin,
                                                           gridSpacing,
                                                           repackNodes);

          scalar_single_attribute_interval_iterator_tests(v);
        }
      }
    }
#endif
  }

  SECTION("multi attribute interval iteration")
  {
    // for a unit cube physical grid [(0,0,0), (1,1,1)]
    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);
    const vec3f gridSpacing(1.f / (128.f - 1.f));

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
    SECTION("structured volumes")
    {
      std::shared_ptr<TestingStructuredVolumeMulti> v(
          generateMultiAttributeStructuredRegularVolume(dimensions,
                                                        gridOrigin,
                                                        gridSpacing,
                                                        TemporalConfig(),
                                                        VKL_DATA_DEFAULT,
                                                        true));

      scalar_multi_attribute_interval_iterator_tests(v);
    }
#endif

#if OPENVKL_DEVICE_CPU_VDB
    SECTION("VDB volumes")
    {
      for (const auto &repackNodes : {true, false}) {
        std::stringstream sectionName;
        sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

        DYNAMIC_SECTION(sectionName.str())
        {
          std::shared_ptr<ProceduralVdbVolumeMulti> v(
              generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                   dimensions,
                                                   gridOrigin,
                                                   gridSpacing,
                                                   repackNodes,
                                                   VKL_DATA_DEFAULT,
                                                   true,
                                                   TemporalConfig()));

          scalar_multi_attribute_interval_iterator_tests(v);
        }
      }
    }
#endif
  }

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
  SECTION("structured volumes: interval nominalDeltaT")
  {
    // use a different volume to facilitate nominalDeltaT tests
    const vec3i dimensions(128);
    const vec3f gridOrigin(-64.f);
    const vec3f gridSpacing(1.f, 2.f, 3.f);

    auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        dimensions, gridOrigin, gridSpacing);

    VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

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
#endif

  shutdownOpenVKL();
}

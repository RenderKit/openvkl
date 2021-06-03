// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "iterator_utility.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized interval iterator", "[interval_iterators]")
{
  initializeOpenVKL();

  int nativeSIMDWidth = vklGetNativeSIMDWidth(getOpenVKLDevice());

  WARN(
      "only performing SIMD vectorized interval iteration tests for width == "
      "native width: "
      << nativeSIMDWidth);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  constexpr int DIMENSION      = 128;
  constexpr int MACROCELL_SIZE = 16;

  const vec3i dimensions(DIMENSION);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(vklSampler);
  vklCommit(intervalContext);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

  const int maxWidth = 16;

  std::array<int, 3> nativeWidths{4, 8, 16};

  SECTION("randomized interval continuity with no value selector")
  {
    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        AlignedVector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        AlignedVector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        AlignedVector<float> tRangesSOA =
            AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize4(intervalContext));
          VKLIntervalIterator4 iterator = vklInitIntervalIterator4(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f4 *)originsSOA.data(),
              (const vkl_vvec3f4 *)directionsSOA.data(),
              (const vkl_vrange1f4 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval4 intervalPrevious, intervalCurrent;
          int result[4];

          int counter = 0;

          while (true) {
            vklIterateInterval4(
                valid.data(), iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize8(intervalContext));
          VKLIntervalIterator8 iterator = vklInitIntervalIterator8(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f8 *)originsSOA.data(),
              (const vkl_vvec3f8 *)directionsSOA.data(),
              (const vkl_vrange1f8 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval8 intervalPrevious, intervalCurrent;
          int result[8];

          int counter = 0;

          while (true) {
            vklIterateInterval8(
                valid.data(), iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize16(intervalContext));
          VKLIntervalIterator16 iterator = vklInitIntervalIterator16(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f16 *)originsSOA.data(),
              (const vkl_vvec3f16 *)directionsSOA.data(),
              (const vkl_vrange1f16 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval16 intervalPrevious, intervalCurrent;
          int result[16];

          int counter = 0;

          while (true) {
            vklIterateInterval16(
                valid.data(), iterator, &intervalCurrent, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i
                   << "] tRange = " << intervalCurrent.tRange.lower[i] << ", "
                   << intervalCurrent.tRange.upper[i]
                   << " nominalDeltaT = " << intervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(intervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(intervalCurrent.tRange.lower[i] ==
                        intervalPrevious.tRange.upper[i]);
              }
            }

            intervalPrevious = intervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(intervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(intervalContext);
  }

  SECTION("randomized interval value ranges with no value selector")
  {
    VKLIntervalIteratorContext intervalContext =
        vklNewIntervalIteratorContext(vklSampler);
    vklCommit(intervalContext);

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        AlignedVector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        AlignedVector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        AlignedVector<float> tRangesSOA =
            AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize4(intervalContext));
          VKLIntervalIterator4 iterator = vklInitIntervalIterator4(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f4 *)originsSOA.data(),
              (const vkl_vvec3f4 *)directionsSOA.data(),
              (const vkl_vrange1f4 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval4 interval;
          int result[4];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval4(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize8(intervalContext));
          VKLIntervalIterator8 iterator = vklInitIntervalIterator8(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f8 *)originsSOA.data(),
              (const vkl_vvec3f8 *)directionsSOA.data(),
              (const vkl_vrange1f8 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval8 interval;
          int result[8];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval8(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize16(intervalContext));
          VKLIntervalIterator16 iterator = vklInitIntervalIterator16(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f16 *)originsSOA.data(),
              (const vkl_vvec3f16 *)directionsSOA.data(),
              (const vkl_vrange1f16 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval16 interval;
          int result[16];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval16(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            if (!result[0]) {
              break;
            }

            for (int i = 0; i < width; i++) {
              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(intervalContext);
  }

  SECTION("randomized interval value ranges with value selector")
  {
    VKLIntervalIteratorContext intervalContext =
        vklNewIntervalIteratorContext(vklSampler);
    vklCommit(intervalContext);

    VKLValueSelector valueSelector = vklNewValueSelector(vklVolume);

    // will trigger intervals covering individual ranges separately
    std::vector<vkl_range1f> valueRanges{{0.9f, 1.f}, {1.9f, 2.f}};

    vklValueSelectorSetRanges(
        valueSelector, valueRanges.size(), valueRanges.data());

    vklCommit(valueSelector);

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        AlignedVector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        AlignedVector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        AlignedVector<float> tRangesSOA =
            AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize4(intervalContext));
          VKLIntervalIterator4 iterator = vklInitIntervalIterator4(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f4 *)originsSOA.data(),
              (const vkl_vvec3f4 *)directionsSOA.data(),
              (const vkl_vrange1f4 *)tRangesSOA.data(),
              valueSelector,
              buffer.data());

          VKLInterval4 interval;
          int result[4];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval4(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize8(intervalContext));
          VKLIntervalIterator8 iterator = vklInitIntervalIterator8(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f8 *)originsSOA.data(),
              (const vkl_vvec3f8 *)directionsSOA.data(),
              (const vkl_vrange1f8 *)tRangesSOA.data(),
              valueSelector,
              buffer.data());

          VKLInterval8 interval;
          int result[8];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval8(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize16(intervalContext));
          VKLIntervalIterator16 iterator = vklInitIntervalIterator16(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f16 *)originsSOA.data(),
              (const vkl_vvec3f16 *)directionsSOA.data(),
              (const vkl_vrange1f16 *)tRangesSOA.data(),
              valueSelector,
              buffer.data());

          VKLInterval16 interval;
          int result[16];

          int intervalCount = 0;

          while (true) {
            vklIterateInterval16(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            if (!resultSum) {
              break;
            }

            for (int i = 0; i < width; i++) {
              if (!result[i]) {
                break;
              }

              INFO("interval iteration "
                   << intervalCount << " lane[" << i
                   << "] tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]
                   << " valueRange = " << interval.valueRange.lower[i] << ", "
                   << interval.valueRange.upper[i]);

              vkl_range1f sampledValueRange = computeIntervalValueRange(
                  vklSampler,
                  reinterpret_cast<vkl_vec3f &>(origins[i]),
                  reinterpret_cast<vkl_vec3f &>(directions[i]),
                  vkl_range1f{interval.tRange.lower[i],
                              interval.tRange.upper[i]});

              INFO("sampled value range = " << sampledValueRange.lower << ", "
                                            << sampledValueRange.upper);

              // the sampled value range should be completely within the
              // returned interval value range
              REQUIRE(
                  (sampledValueRange.lower >= interval.valueRange.lower[i] &&
                   sampledValueRange.upper <= interval.valueRange.upper[i]));

              // the interval value range should overlap the value selector
              // value range(s)
              bool rangeIntersectsValueSelector = false;

              for (const auto &r : valueRanges) {
                if (rangesIntersect(
                        r,
                        vkl_range1f{interval.valueRange.lower[i],
                                    interval.valueRange.upper[i]})) {
                  rangeIntersectsValueSelector = true;
                  break;
                }
              }

              REQUIRE(rangeIntersectsValueSelector);
            }

            intervalCount++;
          }

          // make sure we had at least one interval...
          REQUIRE(intervalCount > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(intervalContext);
    vklRelease(valueSelector);
  }

  SECTION("only write intervals for active lanes")
  {
    VKLIntervalIteratorContext intervalContext =
        vklNewIntervalIteratorContext(vklSampler);
    vklCommit(intervalContext);

    // will be used to initialize all members of interval struct
    constexpr float initialIntervalValue = 999999.f;

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vkl_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vkl_range1f{0.f, inf};
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        AlignedVector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        AlignedVector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        AlignedVector<float> tRangesSOA =
            AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize4(intervalContext));
          VKLIntervalIterator4 iterator = vklInitIntervalIterator4(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f4 *)originsSOA.data(),
              (const vkl_vvec3f4 *)directionsSOA.data(),
              (const vkl_vrange1f4 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval4 interval;

          // initialize interval values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 4; i++) {
            interval.tRange.lower[i]     = initialIntervalValue;
            interval.tRange.upper[i]     = initialIntervalValue;
            interval.valueRange.lower[i] = initialIntervalValue;
            interval.valueRange.upper[i] = initialIntervalValue;
            interval.nominalDeltaT[i]    = initialIntervalValue;
          }

          int result[4];

          int counter = 0;

          while (true) {
            vklIterateInterval4(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i << "] valid = " << valid[i]
                   << ", tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]);

              if (valid[i]) {
                // should have interval values overridden
                REQUIRE(interval.tRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] != initialIntervalValue);
              } else {
                // should NOT have interval values overridden
                REQUIRE(interval.tRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] == initialIntervalValue);
              }
            }

            if (!result[0]) {
              break;
            }

            counter++;
          }

          // ensure at least some intervals were returned
          REQUIRE(counter > 0);
        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize8(intervalContext));
          VKLIntervalIterator8 iterator = vklInitIntervalIterator8(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f8 *)originsSOA.data(),
              (const vkl_vvec3f8 *)directionsSOA.data(),
              (const vkl_vrange1f8 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval8 interval;

          // initialize interval values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 8; i++) {
            interval.tRange.lower[i]     = initialIntervalValue;
            interval.tRange.upper[i]     = initialIntervalValue;
            interval.valueRange.lower[i] = initialIntervalValue;
            interval.valueRange.upper[i] = initialIntervalValue;
            interval.nominalDeltaT[i]    = initialIntervalValue;
          }

          int result[8];

          int counter = 0;

          while (true) {
            vklIterateInterval8(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i << "] valid = " << valid[i]
                   << ", tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]);

              if (valid[i]) {
                // should have interval values overridden
                REQUIRE(interval.tRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] != initialIntervalValue);
              } else {
                // should NOT have interval values overridden
                REQUIRE(interval.tRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] == initialIntervalValue);
              }
            }

            if (!result[0]) {
              break;
            }

            counter++;
          }

          // ensure at least some intervals were returned
          REQUIRE(counter > 0);
        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(
              vklGetIntervalIteratorSize16(intervalContext));
          VKLIntervalIterator16 iterator = vklInitIntervalIterator16(
              valid.data(),
              intervalContext,
              (const vkl_vvec3f16 *)originsSOA.data(),
              (const vkl_vvec3f16 *)directionsSOA.data(),
              (const vkl_vrange1f16 *)tRangesSOA.data(),
              nullptr,
              buffer.data());

          VKLInterval16 interval;

          // initialize interval values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 16; i++) {
            interval.tRange.lower[i]     = initialIntervalValue;
            interval.tRange.upper[i]     = initialIntervalValue;
            interval.valueRange.lower[i] = initialIntervalValue;
            interval.valueRange.upper[i] = initialIntervalValue;
            interval.nominalDeltaT[i]    = initialIntervalValue;
          }

          int result[16];

          int counter = 0;

          while (true) {
            vklIterateInterval16(valid.data(), iterator, &interval, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("interval iteration "
                   << counter << " lane[" << i << "] valid = " << valid[i]
                   << ", tRange = " << interval.tRange.lower[i] << ", "
                   << interval.tRange.upper[i]);

              if (valid[i]) {
                // should have interval values overridden
                REQUIRE(interval.tRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] != initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] != initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] != initialIntervalValue);
              } else {
                // should NOT have interval values overridden
                REQUIRE(interval.tRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.tRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.lower[i] == initialIntervalValue);
                REQUIRE(interval.valueRange.upper[i] == initialIntervalValue);
                REQUIRE(interval.nominalDeltaT[i] == initialIntervalValue);
              }
            }

            if (!result[0]) {
              break;
            }

            counter++;
          }

          // ensure at least some intervals were returned
          REQUIRE(counter > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(intervalContext);
  }

  vklRelease(vklSampler);

  shutdownOpenVKL();
}

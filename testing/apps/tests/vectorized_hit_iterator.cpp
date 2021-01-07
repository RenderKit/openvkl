// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized hit iterator", "[hit_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN(
      "only performing SIMD vectorized hit iteration tests for widths == "
      "native width: "
      << nativeSIMDWidth);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<ZProceduralVolume> v(
      new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  SECTION("vector hit iteration")
  {
    VKLValueSelector valueSelector = vklNewValueSelector(vklVolume);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    vklValueSelectorSetValues(
        valueSelector, isoValues.size(), isoValues.data());

    vklCommit(valueSelector);

    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    const int maxWidth = 16;

    std::array<int, 3> nativeWidths{4, 8, 16};

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

        const std::vector<float> times(callingWidth, 0.f);

        if (callingWidth == 4) {
          std::vector<char> buffer(vklGetIntervalIteratorSize4(vklSampler));
          VKLHitIterator4 iterator = vklInitHitIterator4(valid.data(),
                              vklSampler,
                              (const vkl_vvec3f4 *)originsSOA.data(),
                              (const vkl_vvec3f4 *)directionsSOA.data(),
                              (const vkl_vrange1f4 *)tRangesSOA.data(),
                              times.data(),
                              valueSelector,
                              buffer.data());

          VKLHit4 hit;
          int result[4];

          int hitCount = 0;

          while (true) {
            vklIterateHit4(valid.data(), iterator, &hit, result);

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
              INFO("hit iteration " << hitCount << " lane[" << i
                                    << "] t = " << hit.t[i]
                                    << ", sample =" << hit.sample[i]);

              REQUIRE(hit.t[i] == 1.f + isoValues[hitCount]);
              REQUIRE(hit.sample[i] == isoValues[hitCount]);
            }

            hitCount++;
          }

          REQUIRE(hitCount == isoValues.size());

        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(vklGetIntervalIteratorSize8(vklSampler));
          VKLHitIterator8 iterator = vklInitHitIterator8(valid.data(),
                              vklSampler,
                              (const vkl_vvec3f8 *)originsSOA.data(),
                              (const vkl_vvec3f8 *)directionsSOA.data(),
                              (const vkl_vrange1f8 *)tRangesSOA.data(),
                              times.data(),
                              valueSelector,
                              buffer.data());

          VKLHit8 hit;
          int result[8];

          int hitCount = 0;

          while (true) {
            vklIterateHit8(valid.data(), iterator, &hit, result);

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
              INFO("hit iteration " << hitCount << " lane[" << i
                                    << "] t = " << hit.t[i]
                                    << ", sample =" << hit.sample[i]);

              REQUIRE(hit.t[i] == 1.f + isoValues[hitCount]);
              REQUIRE(hit.sample[i] == isoValues[hitCount]);
            }

            hitCount++;
          }

          REQUIRE(hitCount == isoValues.size());

        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(vklGetIntervalIteratorSize16(vklSampler));
          VKLHitIterator16 iterator = vklInitHitIterator16(valid.data(),
                               vklSampler,
                               (const vkl_vvec3f16 *)originsSOA.data(),
                               (const vkl_vvec3f16 *)directionsSOA.data(),
                               (const vkl_vrange1f16 *)tRangesSOA.data(),
                               times.data(),
                               valueSelector,
                               buffer.data());

          VKLHit16 hit;
          int result[16];

          int hitCount = 0;

          while (true) {
            vklIterateHit16(valid.data(), iterator, &hit, result);

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
              INFO("hit iteration " << hitCount << " lane[" << i
                                    << "] t = " << hit.t[i]
                                    << ", sample =" << hit.sample[i]);

              REQUIRE(hit.t[i] == 1.f + isoValues[hitCount]);
              REQUIRE(hit.sample[i] == isoValues[hitCount]);
            }

            hitCount++;
          }

          REQUIRE(hitCount == isoValues.size());

        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(valueSelector);
  }

  SECTION("only write hits for active lanes")
  {
    // will be used to initialize all members of hit struct
    constexpr float initialHitValue = 999999.f;

    VKLValueSelector valueSelector = vklNewValueSelector(vklVolume);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    vklValueSelectorSetValues(
        valueSelector, isoValues.size(), isoValues.data());

    vklCommit(valueSelector);

    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    const int maxWidth = 16;

    std::array<int, 3> nativeWidths{4, 8, 16};

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

        const std::vector<float> times(callingWidth, 0.f);

        if (callingWidth == 4) {
          std::vector<char> buffer(vklGetIntervalIteratorSize4(vklSampler));
          VKLHitIterator4 iterator = vklInitHitIterator4(valid.data(),
                              vklSampler,
                              (const vkl_vvec3f4 *)originsSOA.data(),
                              (const vkl_vvec3f4 *)directionsSOA.data(),
                              (const vkl_vrange1f4 *)tRangesSOA.data(),
                              times.data(),
                              valueSelector,
                              buffer.data());

          VKLHit4 hit;

          // initialize hit values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 4; i++) {
            hit.t[i]      = initialHitValue;
            hit.sample[i] = initialHitValue;
          }

          int result[4];

          int hitCount = 0;

          while (true) {
            vklIterateHit4(valid.data(), iterator, &hit, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("hit iteration "
                   << hitCount << " lane[" << i << "] valid = " << valid[i]
                   << ", t = " << hit.t[i] << ", sample =" << hit.sample[i]);

              if (valid[i]) {
                // should have hit values overridden
                REQUIRE(hit.t[i] != initialHitValue);
                REQUIRE(hit.sample[i] != initialHitValue);
              } else {
                // should NOT have hit values overridden
                REQUIRE(hit.t[i] == initialHitValue);
                REQUIRE(hit.sample[i] == initialHitValue);
              }
            }

            if (!result[0]) {
              break;
            }

            hitCount++;
          }

          // ensure at least some hits were returned
          REQUIRE(hitCount > 0);
        }

        else if (callingWidth == 8) {
          std::vector<char> buffer(vklGetIntervalIteratorSize8(vklSampler));
          VKLHitIterator8 iterator = vklInitHitIterator8(valid.data(),
                              vklSampler,
                              (const vkl_vvec3f8 *)originsSOA.data(),
                              (const vkl_vvec3f8 *)directionsSOA.data(),
                              (const vkl_vrange1f8 *)tRangesSOA.data(),
                              times.data(),
                              valueSelector,
                              buffer.data());

          VKLHit8 hit;

          // initialize hit values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 8; i++) {
            hit.t[i]      = initialHitValue;
            hit.sample[i] = initialHitValue;
          }

          int result[8];

          int hitCount = 0;

          while (true) {
            vklIterateHit8(valid.data(), iterator, &hit, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("hit iteration "
                   << hitCount << " lane[" << i << "] valid = " << valid[i]
                   << ", t = " << hit.t[i] << ", sample =" << hit.sample[i]);

              if (valid[i]) {
                // should have hit values overridden
                REQUIRE(hit.t[i] != initialHitValue);
                REQUIRE(hit.sample[i] != initialHitValue);
              } else {
                // should NOT have hit values overridden
                REQUIRE(hit.t[i] == initialHitValue);
                REQUIRE(hit.sample[i] == initialHitValue);
              }
            }

            if (!result[0]) {
              break;
            }

            hitCount++;
          }

          // ensure at least some hits were returned
          REQUIRE(hitCount > 0);
        }

        else if (callingWidth == 16) {
          std::vector<char> buffer(vklGetIntervalIteratorSize16(vklSampler));
          VKLHitIterator16 iterator = vklInitHitIterator16(valid.data(),
                               vklSampler,
                               (const vkl_vvec3f16 *)originsSOA.data(),
                               (const vkl_vvec3f16 *)directionsSOA.data(),
                               (const vkl_vrange1f16 *)tRangesSOA.data(),
                               times.data(),
                               valueSelector,
                               buffer.data());

          VKLHit16 hit;

          // initialize hit values; we'll compare after iteration to ensure
          // data was not overridden for inactive lanes
          for (int i = 0; i < 16; i++) {
            hit.t[i]      = initialHitValue;
            hit.sample[i] = initialHitValue;
          }

          int result[16];

          int hitCount = 0;

          while (true) {
            vklIterateHit16(valid.data(), iterator, &hit, result);

            int resultSum = 0;

            for (int i = 0; i < width; i++) {
              resultSum += result[i];
            }

            // we should have the same result for all active lanes
            REQUIRE((resultSum == width || resultSum == 0));

            for (int i = 0; i < callingWidth; i++) {
              INFO("hit iteration "
                   << hitCount << " lane[" << i << "] valid = " << valid[i]
                   << ", t = " << hit.t[i] << ", sample =" << hit.sample[i]);

              if (valid[i]) {
                // should have hit values overridden
                REQUIRE(hit.t[i] != initialHitValue);
                REQUIRE(hit.sample[i] != initialHitValue);
              } else {
                // should NOT have hit values overridden
                REQUIRE(hit.t[i] == initialHitValue);
                REQUIRE(hit.sample[i] == initialHitValue);
              }
            }

            if (!result[0]) {
              break;
            }

            hitCount++;
          }

          // ensure at least some hits were returned
          REQUIRE(hitCount > 0);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease(valueSelector);
  }

  vklRelease(vklSampler);
}

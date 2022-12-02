// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <numeric>
#include "../../external/catch.hpp"
#include "../common/Traits.h"
#include "aos_soa_conversion.h"
#include "common/DeviceTraits.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl;
using namespace openvkl::testing;

// this assumes the procedural z-varying volume with a constant temporal
// configuration
template <int W>
void vector_hit_iteration(VKLSampler vklSampler,
                          VKLHitIteratorContext hitContext,
                          const std::vector<int> &valid,
                          const AlignedVector<float> &originsSOA,
                          const AlignedVector<float> &directionsSOA,
                          const AlignedVector<float> &tRangesSOA,
                          const std::vector<float> &isoValues)
{
  using VKLHitIteratorW       = typename vklDeviceTypes<W>::VKLHitIteratorW;
  using VKLHitW               = typename vklDeviceTypes<W>::VKLHitW;
  auto vklGetHitIteratorSizeW = vklDeviceTypes<W>().vklGetHitIteratorSizeW;
  auto vklInitHitIteratorW    = vklDeviceTypes<W>().vklInitHitIteratorW;
  auto vklIterateHitW         = vklDeviceTypes<W>().vklIterateHitW;

  using vkl_vvec3fW   = typename vklPublicWideTypes<W>::vkl_vvec3fW;
  using vkl_vrange1fW = typename vklPublicWideTypes<W>::vkl_vrange1fW;

  const int numActiveLanes = std::accumulate(valid.begin(), valid.end(), 0);

  std::vector<char> buffer(vklGetHitIteratorSizeW(&hitContext));
  VKLHitIteratorW iterator =
      vklInitHitIteratorW(valid.data(),
                          &hitContext,
                          (const vkl_vvec3fW *)originsSOA.data(),
                          (const vkl_vvec3fW *)directionsSOA.data(),
                          (const vkl_vrange1fW *)tRangesSOA.data(),
                          nullptr,
                          buffer.data());

  VKLHitW hit;
  int result[W];

  int hitCount = 0;

  while (true) {
    vklIterateHitW(valid.data(), iterator, &hit, result);

    int resultSum = 0;

    for (int i = 0; i < W; i++) {
      resultSum += valid[i] ? result[i] : 0;
    }

    // we should have the same result for all active lanes
    REQUIRE((resultSum == numActiveLanes || resultSum == 0));

    if (!result[0]) {
      break;
    }

    for (int i = 0; i < W; i++) {
      if (valid[i]) {
        INFO("hit iteration " << hitCount << " lane[" << i << "] t = "
                              << hit.t[i] << ", sample = " << hit.sample[i]);

        REQUIRE(hit.t[i] == Approx(1.f + isoValues[hitCount]));
        REQUIRE(hit.sample[i] == isoValues[hitCount]);
      }
    }

    hitCount++;
  }

  REQUIRE(hitCount == isoValues.size());
}

// this assumes the procedural z-varying volume, with a non-constant temporal
// configuration
template <int W>
void vector_hit_iteration_time_varying(
    VKLSampler vklSampler,
    VKLHitIteratorContext hitContext,
    const std::vector<int> &valid,
    const AlignedVector<float> &originsSOA,
    const AlignedVector<float> &directionsSOA,
    const AlignedVector<float> &tRangesSOA,
    const std::vector<float> &times,
    const std::vector<float> &isoValues)
{
  using VKLHitIteratorW       = typename vklDeviceTypes<W>::VKLHitIteratorW;
  using VKLHitW               = typename vklDeviceTypes<W>::VKLHitW;
  auto vklGetHitIteratorSizeW = vklDeviceTypes<W>().vklGetHitIteratorSizeW;
  auto vklInitHitIteratorW    = vklDeviceTypes<W>().vklInitHitIteratorW;
  auto vklIterateHitW         = vklDeviceTypes<W>().vklIterateHitW;

  using vkl_vvec3fW   = typename vklPublicWideTypes<W>::vkl_vvec3fW;
  using vkl_vrange1fW = typename vklPublicWideTypes<W>::vkl_vrange1fW;

  std::vector<char> buffer(vklGetHitIteratorSizeW(&hitContext));
  VKLHitIteratorW iterator =
      vklInitHitIteratorW(valid.data(),
                          &hitContext,
                          (const vkl_vvec3fW *)originsSOA.data(),
                          (const vkl_vvec3fW *)directionsSOA.data(),
                          (const vkl_vrange1fW *)tRangesSOA.data(),
                          times.data(),
                          buffer.data());

  VKLHitW hit;
  int result[W];

  int hitCount[W] = {0};

  while (true) {
    vklIterateHitW(valid.data(), iterator, &hit, result);

    int resultSum = 0;

    for (int i = 0; i < W; i++) {
      resultSum += valid[i] ? result[i] : 0;
    }

    if (!resultSum) {
      break;
    }

    for (int i = 0; i < W; i++) {
      if (valid[i] && result[i]) {
        INFO("hit iteration " << hitCount[i] << " lane[" << i
                              << "] time = " << times[i] << ", t = " << hit.t[i]
                              << ", sample = " << hit.sample[i]);

        // procedural function is:  (1.f - time) * objectCoordinates.z;
        REQUIRE(hit.t[i] ==
                Approx(1.f + isoValues[hitCount[i]] / (1.f - times[i])));

        REQUIRE(hit.sample[i] == isoValues[hitCount[i]]);

        hitCount[i]++;
      }
    }
  }

  // check expected hit count per lane
  for (int i = 0; i < W; i++) {
    if (valid[i]) {
      const float maxIsoValueExpected = 1.f - times[i];

      int expectedHitCount = 0;

      for (int j = 0; j < isoValues.size(); j++) {
        if (isoValues[j] <= maxIsoValueExpected) {
          expectedHitCount++;
        }
      }

      INFO("checking expected hit count for lane "
           << i << ", maxIsoValueExpected = " << maxIsoValueExpected);
      REQUIRE(hitCount[i] == expectedHitCount);
    }
  }
}

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
TEST_CASE("Vectorized hit iterator", "[hit_iterators]")
{
  initializeOpenVKL();

  int nativeSIMDWidth = vklGetNativeSIMDWidth(getOpenVKLDevice());

  WARN(
      "only performing SIMD vectorized hit iteration tests for widths == "
      "native width: "
      << nativeSIMDWidth);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  SECTION("vector hit iteration: time constant")
  {
    std::unique_ptr<ZProceduralVolume> v(
        new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

    VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit2(vklSampler);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    VKLData valuesData = vklNewData(
        getOpenVKLDevice(), isoValues.size(), VKL_FLOAT, isoValues.data());

    VKLHitIteratorContext hitContext = vklNewHitIteratorContext(vklSampler);

    vklSetData2(hitContext, "values", valuesData);
    vklRelease(valuesData);

    vklCommit2(hitContext);

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

        if (callingWidth == 4) {
          vector_hit_iteration<4>(vklSampler,
                                  hitContext,
                                  valid,
                                  originsSOA,
                                  directionsSOA,
                                  tRangesSOA,
                                  isoValues);
        }

        else if (callingWidth == 8) {
          vector_hit_iteration<8>(vklSampler,
                                  hitContext,
                                  valid,
                                  originsSOA,
                                  directionsSOA,
                                  tRangesSOA,
                                  isoValues);
        }

        else if (callingWidth == 16) {
          vector_hit_iteration<16>(vklSampler,
                                   hitContext,
                                   valid,
                                   originsSOA,
                                   directionsSOA,
                                   tRangesSOA,
                                   isoValues);
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }

    vklRelease2(hitContext);
    vklRelease2(vklSampler);
  }

  SECTION("vector hit iteration: time varying")
  {
    std::vector<TemporalConfig> temporalConfigs{
        {TemporalConfig::Structured, 4}, {TemporalConfig::Unstructured, 4}};

    for (const auto &temporalConfig : temporalConfigs) {
      std::unique_ptr<ZProceduralVolume> v(new ZProceduralVolume(
          dimensions, gridOrigin, gridSpacing, temporalConfig));

      VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
      VKLSampler vklSampler = vklNewSampler(vklVolume);
      vklCommit2(vklSampler);

      std::vector<float> isoValues;

      for (float f = 0.1f; f < 1.f; f += 0.1f) {
        isoValues.push_back(f);
      }

      VKLData valuesData = vklNewData(
          getOpenVKLDevice(), isoValues.size(), VKL_FLOAT, isoValues.data());

      VKLHitIteratorContext hitContext = vklNewHitIteratorContext(vklSampler);

      vklSetData2(hitContext, "values", valuesData);
      vklRelease(valuesData);

      vklCommit2(hitContext);

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

          AlignedVector<float> originsSOA =
              AOStoSOA_vec3f(origins, callingWidth);
          AlignedVector<float> directionsSOA =
              AOStoSOA_vec3f(directions, callingWidth);
          AlignedVector<float> tRangesSOA =
              AOStoSOA_range1f(tRanges, callingWidth);

          std::vector<float> timesVarying(callingWidth);

          for (int i = 0; i < timesVarying.size(); i++) {
            timesVarying[i] = float(i) / float(callingWidth);
          }

          if (callingWidth == 4) {
            vector_hit_iteration_time_varying<4>(vklSampler,
                                                 hitContext,
                                                 valid,
                                                 originsSOA,
                                                 directionsSOA,
                                                 tRangesSOA,
                                                 timesVarying,
                                                 isoValues);
          }

          else if (callingWidth == 8) {
            vector_hit_iteration_time_varying<8>(vklSampler,
                                                 hitContext,
                                                 valid,
                                                 originsSOA,
                                                 directionsSOA,
                                                 tRangesSOA,
                                                 timesVarying,
                                                 isoValues);
          }

          else if (callingWidth == 16) {
            vector_hit_iteration_time_varying<16>(vklSampler,
                                                  hitContext,
                                                  valid,
                                                  originsSOA,
                                                  directionsSOA,
                                                  tRangesSOA,
                                                  timesVarying,
                                                  isoValues);
          }

          else {
            throw std::runtime_error("unsupported calling width");
          }
        }
      }

      vklRelease2(hitContext);
      vklRelease2(vklSampler);
    }
  }

  SECTION("only write hits for active lanes")
  {
    std::unique_ptr<ZProceduralVolume> v(
        new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

    VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit2(vklSampler);

    std::vector<float> isoValues;

    for (float f = 0.1f; f < 1.f; f += 0.1f) {
      isoValues.push_back(f);
    }

    VKLData valuesData = vklNewData(
        getOpenVKLDevice(), isoValues.size(), VKL_FLOAT, isoValues.data());

    VKLHitIteratorContext hitContext = vklNewHitIteratorContext(vklSampler);

    vklSetData2(hitContext, "values", valuesData);
    vklRelease(valuesData);

    vklCommit2(hitContext);

    // will be used to initialize all members of hit struct
    constexpr float initialHitValue = 999999.f;

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
          std::vector<char> buffer(vklGetHitIteratorSize4(&hitContext));
          VKLHitIterator4 iterator =
              vklInitHitIterator4(valid.data(),
                                  &hitContext,
                                  (const vkl_vvec3f4 *)originsSOA.data(),
                                  (const vkl_vvec3f4 *)directionsSOA.data(),
                                  (const vkl_vrange1f4 *)tRangesSOA.data(),
                                  times.data(),
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
          std::vector<char> buffer(vklGetHitIteratorSize8(&hitContext));
          VKLHitIterator8 iterator =
              vklInitHitIterator8(valid.data(),
                                  &hitContext,
                                  (const vkl_vvec3f8 *)originsSOA.data(),
                                  (const vkl_vvec3f8 *)directionsSOA.data(),
                                  (const vkl_vrange1f8 *)tRangesSOA.data(),
                                  times.data(),
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
          std::vector<char> buffer(vklGetHitIteratorSize16(&hitContext));
          VKLHitIterator16 iterator =
              vklInitHitIterator16(valid.data(),
                                   &hitContext,
                                   (const vkl_vvec3f16 *)originsSOA.data(),
                                   (const vkl_vvec3f16 *)directionsSOA.data(),
                                   (const vkl_vrange1f16 *)tRangesSOA.data(),
                                   times.data(),
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

    vklRelease2(hitContext);
    vklRelease2(vklSampler);
  }

  shutdownOpenVKL();
}
#endif

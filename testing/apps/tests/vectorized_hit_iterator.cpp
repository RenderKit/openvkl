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

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized hit iterator", "[hit_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
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

      for (const int &callingWidth : nativeWidths) {
        if (width > callingWidth || callingWidth != nativeSIMDWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        std::vector<float> originsSOA = AOStoSOA_vec3f(origins, callingWidth);
        std::vector<float> directionsSOA =
            AOStoSOA_vec3f(directions, callingWidth);
        std::vector<float> tRangesSOA = AOStoSOA_range1f(tRanges, callingWidth);

        if (callingWidth == 4) {
          VKLHitIterator4 iterator;
          vklInitHitIterator4(valid.data(),
                              &iterator,
                              vklVolume,
                              (const vkl_vvec3f4 *)originsSOA.data(),
                              (const vkl_vvec3f4 *)directionsSOA.data(),
                              (const vkl_vrange1f4 *)tRangesSOA.data(),
                              valueSelector);

          VKLHit4 hit;
          int result[callingWidth];

          int hitCount = 0;

          while (true) {
            vklIterateHit4(valid.data(), &iterator, &hit, result);

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
          VKLHitIterator8 iterator;
          vklInitHitIterator8(valid.data(),
                              &iterator,
                              vklVolume,
                              (const vkl_vvec3f8 *)originsSOA.data(),
                              (const vkl_vvec3f8 *)directionsSOA.data(),
                              (const vkl_vrange1f8 *)tRangesSOA.data(),
                              valueSelector);

          VKLHit8 hit;
          int result[callingWidth];

          int hitCount = 0;

          while (true) {
            vklIterateHit8(valid.data(), &iterator, &hit, result);

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
          VKLHitIterator16 iterator;
          vklInitHitIterator16(valid.data(),
                               &iterator,
                               vklVolume,
                               (const vkl_vvec3f16 *)originsSOA.data(),
                               (const vkl_vvec3f16 *)directionsSOA.data(),
                               (const vkl_vrange1f16 *)tRangesSOA.data(),
                               valueSelector);

          VKLHit16 hit;
          int result[callingWidth];

          int hitCount = 0;

          while (true) {
            vklIterateHit16(valid.data(), &iterator, &hit, result);

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
  }
}

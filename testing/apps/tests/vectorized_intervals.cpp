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
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized interval iteration")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN(
      "only performing SIMD vectorized surface iteration tests for width == "
      "native width: "
      << nativeSIMDWidth);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  constexpr int DIMENSION      = 128;
  constexpr int MACROCELL_SIZE = 16;

  const vec3i dimensions(DIMENSION);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("randomized interval continuity with no samples mask")
  {
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

        std::vector<float> originsSOA;
        std::vector<float> directionsSOA;
        std::vector<float> tRangesSOA;

        for (int i = 0; i < callingWidth; i++) {
          originsSOA.push_back(i < width ? origins[i].x : 0.f);
          directionsSOA.push_back(i < width ? directions[i].x : 0.f);
          tRangesSOA.push_back(i < width ? tRanges[i].lower : 0.f);
        }

        for (int i = 0; i < callingWidth; i++) {
          originsSOA.push_back(i < width ? origins[i].y : 0.f);
          directionsSOA.push_back(i < width ? directions[i].y : 0.f);
          tRangesSOA.push_back(i < width ? tRanges[i].upper : 0.f);
        }

        for (int i = 0; i < callingWidth; i++) {
          originsSOA.push_back(i < width ? origins[i].z : 0.f);
          directionsSOA.push_back(i < width ? directions[i].z : 0.f);
        }

        // sanity check on SOA conversion
        REQUIRE(originsSOA.size() == callingWidth * 3);
        REQUIRE(directionsSOA.size() == callingWidth * 3);
        REQUIRE(tRangesSOA.size() == callingWidth * 2);

        if (callingWidth == 4) {
          VKLRayIterator4 rayIterator;
          vklInitRayIterator4(valid.data(),
                              &rayIterator,
                              vklVolume,
                              (const vkl_vvec3f4 *)originsSOA.data(),
                              (const vkl_vvec3f4 *)directionsSOA.data(),
                              (const vkl_vrange1f4 *)tRangesSOA.data(),
                              nullptr);

          VKLRayInterval4 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vklIterateInterval4(
                valid.data(), &rayIterator, &rayIntervalCurrent, result);

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
              INFO("rayInterval iteration "
                   << counter << " lane[" << i << "] tRange = "
                   << rayIntervalCurrent.tRange.lower[i] << ", "
                   << rayIntervalCurrent.tRange.upper[i] << " nominalDeltaT = "
                   << rayIntervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(rayIntervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(rayIntervalCurrent.tRange.lower[i] ==
                        rayIntervalPrevious.tRange.upper[i]);
              }
            }

            rayIntervalPrevious = rayIntervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(rayIntervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 8) {
          VKLRayIterator8 rayIterator;
          vklInitRayIterator8(valid.data(),
                              &rayIterator,
                              vklVolume,
                              (const vkl_vvec3f8 *)originsSOA.data(),
                              (const vkl_vvec3f8 *)directionsSOA.data(),
                              (const vkl_vrange1f8 *)tRangesSOA.data(),
                              nullptr);

          VKLRayInterval8 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vklIterateInterval8(
                valid.data(), &rayIterator, &rayIntervalCurrent, result);

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
              INFO("rayInterval iteration "
                   << counter << " lane[" << i << "] tRange = "
                   << rayIntervalCurrent.tRange.lower[i] << ", "
                   << rayIntervalCurrent.tRange.upper[i] << " nominalDeltaT = "
                   << rayIntervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(rayIntervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(rayIntervalCurrent.tRange.lower[i] ==
                        rayIntervalPrevious.tRange.upper[i]);
              }
            }

            rayIntervalPrevious = rayIntervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(rayIntervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else if (callingWidth == 16) {
          VKLRayIterator16 rayIterator;
          vklInitRayIterator16(valid.data(),
                               &rayIterator,
                               vklVolume,
                               (const vkl_vvec3f16 *)originsSOA.data(),
                               (const vkl_vvec3f16 *)directionsSOA.data(),
                               (const vkl_vrange1f16 *)tRangesSOA.data(),
                               nullptr);

          VKLRayInterval16 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vklIterateInterval16(
                valid.data(), &rayIterator, &rayIntervalCurrent, result);

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
              INFO("rayInterval iteration "
                   << counter << " lane[" << i << "] tRange = "
                   << rayIntervalCurrent.tRange.lower[i] << ", "
                   << rayIntervalCurrent.tRange.upper[i] << " nominalDeltaT = "
                   << rayIntervalCurrent.nominalDeltaT[i]);

              if (counter == 0) {
                // first interval at expected beginning
                REQUIRE(rayIntervalCurrent.tRange.lower[i] == 1.f);
              } else {
                // interval continuity
                REQUIRE(rayIntervalCurrent.tRange.lower[i] ==
                        rayIntervalPrevious.tRange.upper[i]);
              }
            }

            rayIntervalPrevious = rayIntervalCurrent;

            counter++;
          }

          // expect a fixed number of intervals to be returned based on
          // structured volume acceleration structure
          REQUIRE(counter == DIMENSION / MACROCELL_SIZE);

          // last interval at expected ending
          for (int i = 0; i < width; i++) {
            REQUIRE(rayIntervalPrevious.tRange.upper[i] == 2.f);
          }
        }

        else {
          throw std::runtime_error("unsupported calling width");
        }
      }
    }
  }
}

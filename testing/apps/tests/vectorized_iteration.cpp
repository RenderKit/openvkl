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
#include "ospray/ospcommon/multidim_index_sequence.h"
#include "volley_testing.h"

using namespace ospcommon;
using namespace volley::testing;

TEST_CASE("Vectorized iteration")
{
  vlyLoadModule("ispc_driver");

  VLYDriver driver = vlyNewDriver("ispc_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  constexpr int DIMENSION      = 128;
  constexpr int MACROCELL_SIZE = 16;

  const vec3i dimensions(DIMENSION);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(dimensions, gridOrigin, gridSpacing));

  VLYVolume vlyVolume = v->getVLYVolume();

  SECTION("randomized interval continuity with no samples mask")
  {
    vly_box3f bbox = vlyGetBoundingBox(vlyVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    const int maxWidth = 16;

    std::array<int, 3> nativeWidths{4, 8, 16};

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> origins(width);
      std::vector<vec3f> directions(width);
      std::vector<vly_range1f> tRanges(width);

      for (int i = 0; i < width; i++) {
        origins[i]    = vec3f(distX(eng), distY(eng), -1.f);
        directions[i] = vec3f(0.f, 0.f, 1.f);
        tRanges[i]    = vly_range1f{0.f, inf};
      }

      for (const int &callingWidth : nativeWidths) {
        if (width > callingWidth) {
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
          VLYRayIterator rayIterator =
              vlyNewRayIterator4(valid.data(),
                                 vlyVolume,
                                 (const vly_vvec3f4 *)originsSOA.data(),
                                 (const vly_vvec3f4 *)directionsSOA.data(),
                                 (const vly_vrange1f4 *)tRangesSOA.data(),
                                 nullptr);

          VLYRayInterval4 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vlyIterateInterval4(
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
          VLYRayIterator rayIterator =
              vlyNewRayIterator8(valid.data(),
                                 vlyVolume,
                                 (const vly_vvec3f8 *)originsSOA.data(),
                                 (const vly_vvec3f8 *)directionsSOA.data(),
                                 (const vly_vrange1f8 *)tRangesSOA.data(),
                                 nullptr);

          VLYRayInterval8 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vlyIterateInterval8(
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
          VLYRayIterator rayIterator =
              vlyNewRayIterator16(valid.data(),
                                  vlyVolume,
                                  (const vly_vvec3f16 *)originsSOA.data(),
                                  (const vly_vvec3f16 *)directionsSOA.data(),
                                  (const vly_vrange1f16 *)tRangesSOA.data(),
                                  nullptr);

          VLYRayInterval16 rayIntervalPrevious, rayIntervalCurrent;
          int result[callingWidth];

          int counter = 0;

          while (true) {
            vlyIterateInterval16(
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

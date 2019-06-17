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
#include "ospcommon/utility/multidim_index_sequence.h"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;

TEST_CASE("Vectorized sampling")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("randomized vectorized sampling varying calling width and masks")
  {
    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
    std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

    const int maxWidth = 16;

    std::array<int, 3> nativeWidths{4, 8, 16};

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> objectCoordinates(width);
      for (auto &oc : objectCoordinates) {
        oc = vec3f(distX(eng), distY(eng), distZ(eng));
      }

      for (const int &callingWidth : nativeWidths) {
        if (width > callingWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        std::vector<float> objectCoordinatesSOA;

        for (int i = 0; i < callingWidth; i++)
          objectCoordinatesSOA.push_back(i < width ? objectCoordinates[i].x
                                                   : 0.f);
        for (int i = 0; i < callingWidth; i++)
          objectCoordinatesSOA.push_back(i < width ? objectCoordinates[i].y
                                                   : 0.f);
        for (int i = 0; i < callingWidth; i++)
          objectCoordinatesSOA.push_back(i < width ? objectCoordinates[i].z
                                                   : 0.f);

        // sanity check on SOA conversion
        REQUIRE(objectCoordinatesSOA.size() == callingWidth * 3);

        float samples[callingWidth];

        if (callingWidth == 4) {
          vklComputeSample4(valid.data(),
                            vklVolume,
                            (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                            samples);
        } else if (callingWidth == 8) {
          vklComputeSample8(valid.data(),
                            vklVolume,
                            (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                            samples);

        } else if (callingWidth == 16) {
          vklComputeSample16(valid.data(),
                             vklVolume,
                             (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                             samples);
        } else {
          throw std::runtime_error("unsupported calling width");
        }

        for (int i = 0; i < width; i++) {
          float sampleTruth = vklComputeSample(
              vklVolume, (const vkl_vec3f *)&objectCoordinates[i]);

          INFO("sample = " << i + 1 << " / " << width
                           << ", calling width = " << callingWidth);
          REQUIRE(sampleTruth == samples[i]);
        }
      }
    }
  }
}

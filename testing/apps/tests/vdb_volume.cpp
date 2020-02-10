// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace openvkl;

static void init_driver()
{
  vklLoadModule("ispc_driver");
  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

using openvkl::testing::WaveletVdbVolume;

TEST_CASE("VDB volume value range", "[value_range]")
{
  init_driver();

  SECTION("WaveletVdbVolume nearest")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_NEAREST));
    REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
    REQUIRE(valueRange.upper >= valueRange.lower);
    REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
            Approx(6.f).epsilon(0.001f));
    REQUIRE_NOTHROW(delete volume);
  }

  SECTION("WaveletVdbVolume trilinear")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));
    REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
    REQUIRE(valueRange.upper >= valueRange.lower);
    REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
            Approx(6.f).epsilon(0.001f));
    REQUIRE_NOTHROW(delete volume);
  }
}

TEST_CASE("VDB volume sampling", "[volume_sampling]")
{
  init_driver();

  SECTION("WaveletVdbVolume nearest")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_NEAREST));

    VKLVolume vklVolume = volume->getVKLVolume();
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;

      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      const float proceduralValue =
          volume->computeProceduralValue(objectCoordinates);

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      test_scalar_and_vector_sampling(
          vklVolume, objectCoordinates, proceduralValue, 1e-4f);
    }

    REQUIRE_NOTHROW(delete volume);
  }

  SECTION("WaveletVdbVolume trilinear")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

    VKLVolume vklVolume = volume->getVKLVolume();
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;

      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      const float proceduralValue =
          volume->computeProceduralValue(objectCoordinates);

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      test_scalar_and_vector_sampling(
          vklVolume, objectCoordinates, proceduralValue, 1e-4f);
    }

    REQUIRE_NOTHROW(delete volume);
  }
}

TEST_CASE("VDB volume interval iterator", "[volume_sampling]")
{
  init_driver();

  WaveletVdbVolume *volume = nullptr;
  range1f valueRange;
  REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                      128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

  VKLVolume vklVolume = volume->getVKLVolume();
  VKLIntervalIterator iterator;
  VKLInterval interval;
  vkl_vec3f origin{0, 0, -5.f};
  vkl_vec3f direction{0, 0, 1.f};
  vkl_range1f tRange{0.f, 1000.f};
  REQUIRE_NOTHROW(vklInitIntervalIterator(
      &iterator, vklVolume, &origin, &direction, &tRange, nullptr));
  REQUIRE_NOTHROW(vklIterateInterval(&iterator, &interval));
}

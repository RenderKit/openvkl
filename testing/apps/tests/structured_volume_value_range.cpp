// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

using namespace ospcommon;
using namespace openvkl::testing;

template <typename PROCEDURAL_VOLUME_TYPE>
void computed_vs_api_value_range(vec3i dimensions = vec3i(128))
{
  const float boundingBoxSize = 2.f;

  vec3f gridOrigin;
  vec3f gridSpacing;

  // generate legal grid parameters
  PROCEDURAL_VOLUME_TYPE::generateGridParameters(
      dimensions, boundingBoxSize, gridOrigin, gridSpacing);

  auto v = ospcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_range1f apiValueRange = vklGetValueRange(vklVolume);

  range1f computedValueRange = v->getComputedValueRange();

  INFO("api valueRange = " << apiValueRange.lower << " "
                           << apiValueRange.upper);
  INFO("computed valueRange = " << computedValueRange.lower << " "
                                << computedValueRange.upper);

  REQUIRE((apiValueRange.lower == computedValueRange.lower &&
           apiValueRange.upper == computedValueRange.upper));
}

TEST_CASE("Structured volume value range", "[volume_value_range]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("unsigned char")
  {
    computed_vs_api_value_range<WaveletStructuredRegularVolumeUChar>();
    computed_vs_api_value_range<WaveletStructuredSphericalVolumeUChar>();
  }

  SECTION("short")
  {
    computed_vs_api_value_range<WaveletStructuredRegularVolumeShort>();
    computed_vs_api_value_range<WaveletStructuredSphericalVolumeShort>();
  }

  SECTION("unsigned short")
  {
    computed_vs_api_value_range<WaveletStructuredRegularVolumeUShort>();
    computed_vs_api_value_range<WaveletStructuredSphericalVolumeUShort>();
  }

  SECTION("float")
  {
    computed_vs_api_value_range<WaveletStructuredRegularVolumeFloat>();
    computed_vs_api_value_range<WaveletStructuredSphericalVolumeFloat>();
  }

  SECTION("double")
  {
    computed_vs_api_value_range<WaveletStructuredRegularVolumeDouble>();
    computed_vs_api_value_range<WaveletStructuredSphericalVolumeDouble>();
  }
}

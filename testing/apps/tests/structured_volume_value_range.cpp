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

template <typename VOXEL_TYPE>
void computed_vs_api_value_range(vec3i dimensions)
{
  std::unique_ptr<
      ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                        getWaveletValue<VOXEL_TYPE>>>
      v(new ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                              getWaveletValue<VOXEL_TYPE>>(
          dimensions, vec3f(0.f), vec3f(1.f)));

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
    computed_vs_api_value_range<unsigned char>(vec3i(128));
  }

  SECTION("short")
  {
    computed_vs_api_value_range<short>(vec3i(128));
  }

  SECTION("unsigned short")
  {
    computed_vs_api_value_range<unsigned short>(vec3i(128));
  }

  SECTION("float")
  {
    computed_vs_api_value_range<float>(vec3i(128));
  }

  SECTION("double")
  {
    computed_vs_api_value_range<double>(vec3i(128));
  }
}

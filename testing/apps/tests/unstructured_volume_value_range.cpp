// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

void computed_vs_api_value_range(vec3i dimensions,
                                 VKLUnstructuredCellType primType)
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          dimensions, vec3f(0.f), vec3f(1.f), primType, true));

  VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

  vkl_range1f apiValueRange = vklGetValueRange(vklVolume);

  range1f computedValueRange = v->getComputedValueRange();

  INFO("api valueRange = " << apiValueRange.lower << " "
                           << apiValueRange.upper);
  INFO("computed valueRange = " << computedValueRange.lower << " "
                                << computedValueRange.upper);

  REQUIRE((apiValueRange.lower == computedValueRange.lower &&
           apiValueRange.upper == computedValueRange.upper));
}

TEST_CASE("Unstructured volume value range", "[volume_value_range]")
{
  initializeOpenVKL();

  SECTION("hexahedron")
  {
    computed_vs_api_value_range(vec3i(128), VKL_HEXAHEDRON);
  }

  SECTION("tetrahedron")
  {
    computed_vs_api_value_range(vec3i(128), VKL_TETRAHEDRON);
  }

  SECTION("wedge")
  {
    computed_vs_api_value_range(vec3i(128), VKL_WEDGE);
  }

  SECTION("pyramid")
  {
    computed_vs_api_value_range(vec3i(128), VKL_PYRAMID);
  }

  shutdownOpenVKL();
}

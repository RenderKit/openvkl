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

#include "../../external/catch.hpp"
#include "common/simd.h"
#include "volley/drivers/ispc/GridAcceleratorRayIterator_ispc.h"
#include "volley/drivers/ispc/iterator/GridAcceleratorRayIterator.h"
#include "volley/drivers/ispc/simd_conformance_ispc.h"
#include "volley_testing.h"

using namespace ospcommon;
using namespace volley::testing;
using namespace volley;

template <int W>
void vrange1fn_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vrange1fn<W>));
  REQUIRE(sizeof(vrange1fn<W>) == ispc::sizeofVaryingRange1f());
  REQUIRE(is_aligned_for_type<vrange1fn<W>>(ispc::newVaryingRange1f()));
};

template <int W>
void vvec3fn_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vvec3fn<W>));
  REQUIRE(sizeof(vvec3fn<W>) == ispc::sizeofVaryingVec3f());
  REQUIRE(is_aligned_for_type<vvec3fn<W>>(ispc::newVaryingVec3f()));
}

template <int W>
void vVLYRayIntervalN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVLYRayIntervalN<W>));
  REQUIRE(sizeof(vVLYRayIntervalN<W>) == ispc::sizeofVaryingRayInterval());
  REQUIRE(
      is_aligned_for_type<vVLYRayIntervalN<W>>(ispc::newVaryingRayInterval()));
}

template <int W>
void vVLYSurfaceHitN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVLYSurfaceHitN<W>));
  REQUIRE(sizeof(vVLYSurfaceHitN<W>) == ispc::sizeofVaryingSurfaceHit());
  REQUIRE(
      is_aligned_for_type<vVLYSurfaceHitN<W>>(ispc::newVaryingSurfaceHit()));
}

TEST_CASE("SIMD conformance")
{
  vlyLoadModule("ispc_driver");

  VLYDriver driver = vlyNewDriver("ispc_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  int nativeSIMDWidth = vlyGetNativeSIMDWidth();

  WARN("only performing SIMD conformance tests for native width: "
       << nativeSIMDWidth);

  if (nativeSIMDWidth == 4) {
    SECTION("4-wide")
    {
      vrange1fn_conformance_test<4>();
      vvec3fn_conformance_test<4>();
      vVLYRayIntervalN_conformance_test<4>();
      vVLYSurfaceHitN_conformance_test<4>();
    }
  }

  else if (nativeSIMDWidth == 8) {
    SECTION("8-wide")
    {
      vrange1fn_conformance_test<8>();
      vvec3fn_conformance_test<8>();
      vVLYRayIntervalN_conformance_test<8>();
      vVLYSurfaceHitN_conformance_test<8>();
    }
  }

  else if (nativeSIMDWidth == 16) {
    SECTION("16-wide")
    {
      vrange1fn_conformance_test<16>();
      vvec3fn_conformance_test<16>();
      vVLYRayIntervalN_conformance_test<16>();
      vVLYSurfaceHitN_conformance_test<16>();
    }
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }

  SECTION("GridAcceleratorRayIterator sizing (16-wide only)")
  {
    if (nativeSIMDWidth != 16) {
      WARN(
          "not performing ray iterator sizing test; test only legal on 16-wide "
          "runtime width");
    } else {
      int ispcSize = ispc::GridAcceleratorRayIterator_sizeOf();
      REQUIRE(sizeof(volley::ispc_driver::GridAcceleratorRayIterator<16>) <=
              RAY_ITERATOR_INTERNAL_STATE_SIZE);
      REQUIRE(ispcSize == ISPC_STORAGE_SIZE);
    }
  }
}

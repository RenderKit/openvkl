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
#include "openvkl/drivers/ispc/GridAcceleratorRayIterator_ispc.h"
#include "openvkl/drivers/ispc/iterator/GridAcceleratorRayIterator.h"
#include "openvkl/drivers/ispc/simd_conformance_ispc.h"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;
using namespace openvkl;

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
void vVKLRayIteratorN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLRayIteratorN<W>));

  REQUIRE(sizeof(vVKLRayIteratorN<W>) == ispc::sizeofUniformVKLRayIterator());
  REQUIRE(is_aligned_for_type<vVKLRayIteratorN<W>>(
      ispc::newUniformVKLRayIterator()));

  if (W == 4) {
    REQUIRE(sizeof(VKLRayIterator4) == sizeof(vVKLRayIteratorN<W>));
    REQUIRE(alignof(VKLRayIterator4) == alignof(vVKLRayIteratorN<W>));
  } else if (W == 8) {
    REQUIRE(sizeof(VKLRayIterator8) == sizeof(vVKLRayIteratorN<W>));
    REQUIRE(alignof(VKLRayIterator8) == alignof(vVKLRayIteratorN<W>));
  } else if (W == 16) {
    REQUIRE(sizeof(VKLRayIterator16) == sizeof(vVKLRayIteratorN<W>));
    REQUIRE(alignof(VKLRayIterator16) == alignof(vVKLRayIteratorN<W>));

    // special case: scalar ray iterator should match size of maximum width
    // (16); alignment doesn't matter since the conversions make copies.
    REQUIRE(sizeof(VKLRayIterator) == sizeof(vVKLRayIteratorN<W>));
  } else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

template <int W>
void vVKLRayIntervalN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLRayIntervalN<W>));
  REQUIRE(sizeof(vVKLRayIntervalN<W>) == ispc::sizeofVaryingRayInterval());
  REQUIRE(
      is_aligned_for_type<vVKLRayIntervalN<W>>(ispc::newVaryingRayInterval()));
}

template <int W>
void vVKLSurfaceHitN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLSurfaceHitN<W>));
  REQUIRE(sizeof(vVKLSurfaceHitN<W>) == ispc::sizeofVaryingSurfaceHit());
  REQUIRE(
      is_aligned_for_type<vVKLSurfaceHitN<W>>(ispc::newVaryingSurfaceHit()));
}

template <int W>
void GridAcceleratorRayIterator_conformance_test()
{
  int ispcSize = ispc::GridAcceleratorRayIterator_sizeOf();
  REQUIRE(ispcSize ==
          openvkl::ispc_driver::GridAcceleratorRayIterator<W>::ispcStorageSize);

  REQUIRE(
      is_aligned_for_type<openvkl::ispc_driver::GridAcceleratorRayIterator<W>>(
          ispc::GridAcceleratorRayIterator_new()));

  REQUIRE(sizeof(openvkl::ispc_driver::GridAcceleratorRayIterator<W>) <=
          ray_iterator_internal_state_size_for_width(W));
}

TEST_CASE("SIMD conformance")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN("only performing SIMD conformance tests for native width: "
       << nativeSIMDWidth);

  if (nativeSIMDWidth == 4) {
    SECTION("4-wide")
    {
      vrange1fn_conformance_test<4>();
      vvec3fn_conformance_test<4>();
      vVKLRayIteratorN_conformance_test<4>();
      vVKLRayIntervalN_conformance_test<4>();
      vVKLSurfaceHitN_conformance_test<4>();
      GridAcceleratorRayIterator_conformance_test<4>();
    }
  }

  else if (nativeSIMDWidth == 8) {
    SECTION("8-wide")
    {
      vrange1fn_conformance_test<8>();
      vvec3fn_conformance_test<8>();
      vVKLRayIteratorN_conformance_test<8>();
      vVKLRayIntervalN_conformance_test<8>();
      vVKLSurfaceHitN_conformance_test<8>();
      GridAcceleratorRayIterator_conformance_test<8>();
    }
  }

  else if (nativeSIMDWidth == 16) {
    SECTION("16-wide")
    {
      vrange1fn_conformance_test<16>();
      vvec3fn_conformance_test<16>();
      vVKLRayIteratorN_conformance_test<16>();
      vVKLRayIntervalN_conformance_test<16>();
      vVKLSurfaceHitN_conformance_test<16>();
      GridAcceleratorRayIterator_conformance_test<16>();
    }
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

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
#include "../common/simd.h"
#include "openvkl/drivers/ispc/iterator/GridAcceleratorIterator.h"
#include "simd_conformance_ispc.h"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;
using namespace openvkl;

template <int W>
void driver_native_simd_width_conformance_test()
{
  INFO("width = " << W);
  REQUIRE(vklGetNativeSIMDWidth() == W);
}

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
void vVKLIntervalIteratorN_conformance_test()
{
  INFO("width = " << W
                  << ", alignment = " << alignof(vVKLIntervalIteratorN<W>));

  REQUIRE(sizeof(vVKLIntervalIteratorN<W>) ==
          ispc::sizeofVaryingVKLIntervalIterator());
  REQUIRE(is_aligned_for_type<vVKLIntervalIteratorN<W>>(
      ispc::newVaryingVKLIntervalIterator()));

  if (W == 4) {
    REQUIRE(sizeof(VKLIntervalIterator4) == sizeof(vVKLIntervalIteratorN<W>));
    REQUIRE(alignof(VKLIntervalIterator4) == alignof(vVKLIntervalIteratorN<W>));
  } else if (W == 8) {
    REQUIRE(sizeof(VKLIntervalIterator8) == sizeof(vVKLIntervalIteratorN<W>));
    REQUIRE(alignof(VKLIntervalIterator8) == alignof(vVKLIntervalIteratorN<W>));
  } else if (W == 16) {
    REQUIRE(sizeof(VKLIntervalIterator16) == sizeof(vVKLIntervalIteratorN<W>));
    REQUIRE(alignof(VKLIntervalIterator16) ==
            alignof(vVKLIntervalIteratorN<W>));

    // special case: scalar ray iterator should match size of maximum width
    // (16); alignment doesn't matter since the conversions make copies.
    REQUIRE(sizeof(VKLIntervalIterator) == sizeof(vVKLIntervalIteratorN<W>));
  } else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

template <int W>
void vVKLHitIteratorN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLHitIteratorN<W>));

  REQUIRE(sizeof(vVKLHitIteratorN<W>) == ispc::sizeofVaryingVKLHitIterator());
  REQUIRE(is_aligned_for_type<vVKLHitIteratorN<W>>(
      ispc::newVaryingVKLHitIterator()));

  if (W == 4) {
    REQUIRE(sizeof(VKLHitIterator4) == sizeof(vVKLHitIteratorN<W>));
    REQUIRE(alignof(VKLHitIterator4) == alignof(vVKLHitIteratorN<W>));
  } else if (W == 8) {
    REQUIRE(sizeof(VKLHitIterator8) == sizeof(vVKLHitIteratorN<W>));
    REQUIRE(alignof(VKLHitIterator8) == alignof(vVKLHitIteratorN<W>));
  } else if (W == 16) {
    REQUIRE(sizeof(VKLHitIterator16) == sizeof(vVKLHitIteratorN<W>));
    REQUIRE(alignof(VKLHitIterator16) == alignof(vVKLHitIteratorN<W>));

    // special case: scalar ray iterator should match size of maximum width
    // (16); alignment doesn't matter since the conversions make copies.
    REQUIRE(sizeof(VKLHitIterator) == sizeof(vVKLHitIteratorN<W>));
  } else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

template <int W>
void vVKLIntervalN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLIntervalN<W>));
  REQUIRE(sizeof(vVKLIntervalN<W>) == ispc::sizeofVaryingInterval());
  REQUIRE(is_aligned_for_type<vVKLIntervalN<W>>(ispc::newVaryingInterval()));
}

template <int W>
void vVKLHitN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLHitN<W>));
  REQUIRE(sizeof(vVKLHitN<W>) == ispc::sizeofVaryingHit());
  REQUIRE(is_aligned_for_type<vVKLHitN<W>>(ispc::newVaryingHit()));
}

template <int W>
void GridAcceleratorIterator_conformance_test()
{
  int ispcSize = ispc::sizeofGridAcceleratorIterator();
  REQUIRE(ispcSize ==
          openvkl::ispc_driver::GridAcceleratorIterator<W>::ispcStorageSize);

  REQUIRE(is_aligned_for_type<openvkl::ispc_driver::GridAcceleratorIterator<W>>(
      ispc::newGridAcceleratorIterator()));

  REQUIRE(sizeof(openvkl::ispc_driver::GridAcceleratorIterator<W>) <=
          iterator_internal_state_size_for_width(W));
}

TEST_CASE("SIMD conformance", "[simd_conformance]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN("only performing SIMD conformance tests for native width: "
       << nativeSIMDWidth);

  if (nativeSIMDWidth == 4) {
    SECTION("4-wide")
    {
      driver_native_simd_width_conformance_test<4>();
      vrange1fn_conformance_test<4>();
      vvec3fn_conformance_test<4>();
      vVKLIntervalIteratorN_conformance_test<4>();
      vVKLHitIteratorN_conformance_test<4>();
      vVKLIntervalN_conformance_test<4>();
      vVKLHitN_conformance_test<4>();
      GridAcceleratorIterator_conformance_test<4>();
    }
  }

  else if (nativeSIMDWidth == 8) {
    SECTION("8-wide")
    {
      driver_native_simd_width_conformance_test<8>();
      vrange1fn_conformance_test<8>();
      vvec3fn_conformance_test<8>();
      vVKLIntervalIteratorN_conformance_test<8>();
      vVKLHitIteratorN_conformance_test<8>();
      vVKLIntervalN_conformance_test<8>();
      vVKLHitN_conformance_test<8>();
      GridAcceleratorIterator_conformance_test<8>();
    }
  }

  else if (nativeSIMDWidth == 16) {
    SECTION("16-wide")
    {
      driver_native_simd_width_conformance_test<16>();
      vrange1fn_conformance_test<16>();
      vvec3fn_conformance_test<16>();
      vVKLIntervalIteratorN_conformance_test<16>();
      vVKLHitIteratorN_conformance_test<16>();
      vVKLIntervalN_conformance_test<16>();
      vVKLHitN_conformance_test<16>();
      GridAcceleratorIterator_conformance_test<16>();
    }
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

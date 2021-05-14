// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "../common/simd.h"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;
using namespace openvkl;

template <int sourceWidth, int destWidth>
void vvec3fn_upconversion_test()
{
  vvec3fn<sourceWidth> source;

  for (int i = 0; i < sourceWidth; i++) {
    source.x[i] = i * 3 + 0;
    source.y[i] = i * 3 + 1;
    source.z[i] = i * 3 + 2;
  }

  vvec3fn<destWidth> dest = static_cast<vvec3fn<destWidth>>(source);

  INFO("source width = " << sourceWidth << ", dest width = " << destWidth);

  for (int i = 0; i < sourceWidth; i++) {
    REQUIRE(source.x[i] == dest.x[i]);
    REQUIRE(source.y[i] == dest.y[i]);
    REQUIRE(source.z[i] == dest.z[i]);
  }
};

template <int sourceWidth, int packWidth>
void vvec3fn_pack_extraction_test()
{
  vvec3fn<sourceWidth> source;

  for (int i = 0; i < sourceWidth; i++) {
    source.x[i] = i * 3 + 0;
    source.y[i] = i * 3 + 1;
    source.z[i] = i * 3 + 2;
  }

  const int numPacks = sourceWidth / packWidth + (sourceWidth % packWidth != 0);

  for (int packIndex = 0; packIndex < numPacks; packIndex++) {
    vvec3fn<packWidth> pack =
        source.template extract_pack<packWidth>(packIndex);

    INFO("source width = " << sourceWidth << ", pack width = " << packWidth
                           << ", pack index = " << packIndex);

    for (int i = packIndex * packWidth;
         i < (packIndex + 1) * packWidth && i < sourceWidth;
         i++) {
      REQUIRE(source.x[i] == pack.x[i - packIndex * packWidth]);
    }
  }
};

TEST_CASE("SIMD type conversion", "[simd_conformance]")
{
  initializeOpenVKL();

  SECTION("vvec3fn upconversion")
  {
    vvec3fn_upconversion_test<4, 4>();
    vvec3fn_upconversion_test<4, 8>();
    vvec3fn_upconversion_test<4, 16>();

    vvec3fn_upconversion_test<8, 8>();
    vvec3fn_upconversion_test<8, 16>();

    vvec3fn_upconversion_test<16, 16>();
  }

  SECTION("vvec3fn pack extraction")
  {
    vvec3fn_pack_extraction_test<16, 16>();
    vvec3fn_pack_extraction_test<16, 8>();
    vvec3fn_pack_extraction_test<16, 4>();

    vvec3fn_pack_extraction_test<8, 8>();
    vvec3fn_pack_extraction_test<8, 4>();

    vvec3fn_pack_extraction_test<4, 4>();

    // the below are not currently used, but should still work...
    vvec3fn_pack_extraction_test<15, 16>();
    vvec3fn_pack_extraction_test<15, 8>();
    vvec3fn_pack_extraction_test<15, 4>();

    vvec3fn_pack_extraction_test<14, 16>();
    vvec3fn_pack_extraction_test<14, 8>();
    vvec3fn_pack_extraction_test<14, 4>();

    vvec3fn_pack_extraction_test<13, 16>();
    vvec3fn_pack_extraction_test<13, 8>();
    vvec3fn_pack_extraction_test<13, 4>();

    vvec3fn_pack_extraction_test<12, 16>();
    vvec3fn_pack_extraction_test<12, 8>();
    vvec3fn_pack_extraction_test<12, 4>();
  }

  shutdownOpenVKL();
}

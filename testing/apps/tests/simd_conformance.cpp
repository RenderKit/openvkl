// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <algorithm>
#include "../../external/catch.hpp"
#include "../common/Traits.h"
#include "../common/simd.h"
#include "openvkl/common.h"
#include "openvkl_testing.h"
#include "simd_conformance_ispc.h"

using namespace rkcommon;
using namespace openvkl::testing;
using namespace openvkl;

template <int W>
void public_wide_types_conformance_test()
{
  INFO("width = " << W);

  using vkl_vvec3fW   = typename vklPublicWideTypes<W>::vkl_vvec3fW;
  using vkl_vrange1fW = typename vklPublicWideTypes<W>::vkl_vrange1fW;
  using VKLIntervalW  = typename vklPublicWideTypes<W>::VKLIntervalW;
  using VKLHitW       = typename vklPublicWideTypes<W>::VKLHitW;

  REQUIRE(sizeof(vvec3fn<W>) == sizeof(vkl_vvec3fW));
  REQUIRE(alignof(vvec3fn<W>) == alignof(vkl_vvec3fW));

  REQUIRE(sizeof(vrange1fn<W>) == sizeof(vkl_vrange1fW));
  REQUIRE(alignof(vrange1fn<W>) == alignof(vkl_vrange1fW));

  REQUIRE(sizeof(vVKLIntervalN<W>) == sizeof(VKLIntervalW));
  REQUIRE(alignof(vVKLIntervalN<W>) == alignof(VKLIntervalW));

  REQUIRE(sizeof(vVKLHitN<W>) == sizeof(VKLHitW));
  REQUIRE(alignof(vVKLHitN<W>) == alignof(VKLHitW));
}

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
  void *ptr = ispc::newVaryingRange1f();
  REQUIRE(is_aligned_for_type<vrange1fn<W>>(ptr));
  ispc::delete_uniform(ptr);
};

template <int W>
void vvec3fn_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vvec3fn<W>));
  REQUIRE(sizeof(vvec3fn<W>) == ispc::sizeofVaryingVec3f());
  void *ptr = ispc::newVaryingVec3f();
  REQUIRE(is_aligned_for_type<vvec3fn<W>>(ptr));
  ispc::delete_uniform(ptr);
}

template <int W>
void vVKLIntervalN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLIntervalN<W>));
  REQUIRE(sizeof(vVKLIntervalN<W>) == ispc::sizeofVaryingInterval());
  void *ptr = ispc::newVaryingInterval();
  REQUIRE(is_aligned_for_type<vVKLIntervalN<W>>(ptr));
  ispc::delete_uniform(ptr);
}

template <int W>
void vVKLHitN_conformance_test()
{
  INFO("width = " << W << ", alignment = " << alignof(vVKLHitN<W>));
  REQUIRE(sizeof(vVKLHitN<W>) == ispc::sizeofVaryingHit());
  void *ptr = ispc::newVaryingHit();
  REQUIRE(is_aligned_for_type<vVKLHitN<W>>(ptr));
  ispc::delete_uniform(ptr);
}

TEST_CASE("SIMD conformance", "[simd_conformance]")
{
  // verifies public wide types vs internal wide representations, e.g.
  // vkl_vvecef16 vs vvec3fn<16>
  public_wide_types_conformance_test<1>();
  public_wide_types_conformance_test<4>();
  public_wide_types_conformance_test<8>();
  public_wide_types_conformance_test<16>();

  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  int nativeSIMDWidth = vklGetNativeSIMDWidth();

  WARN("only performing ISPC-side SIMD conformance tests for native width: "
       << nativeSIMDWidth);

  if (nativeSIMDWidth == 4) {
#if VKL_TARGET_WIDTH_ENABLED_4
    SECTION("native (4-wide)")
    {
      driver_native_simd_width_conformance_test<4>();
      vrange1fn_conformance_test<4>();
      vvec3fn_conformance_test<4>();
      vVKLIntervalN_conformance_test<4>();
      vVKLHitN_conformance_test<4>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else if (nativeSIMDWidth == 8) {
#if VKL_TARGET_WIDTH_ENABLED_8
    SECTION("native (8-wide)")
    {
      driver_native_simd_width_conformance_test<8>();
      vrange1fn_conformance_test<8>();
      vvec3fn_conformance_test<8>();
      vVKLIntervalN_conformance_test<8>();
      vVKLHitN_conformance_test<8>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else if (nativeSIMDWidth == 16) {
#if VKL_TARGET_WIDTH_ENABLED_16
    SECTION("native (16-wide)")
    {
      driver_native_simd_width_conformance_test<16>();
      vrange1fn_conformance_test<16>();
      vvec3fn_conformance_test<16>();
      vVKLIntervalN_conformance_test<16>();
      vVKLHitN_conformance_test<16>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

template <int W, int INTERVAL_MACRO, int HIT_MACRO>
void max_iterator_size_conformance_test()
{
  size_t maxIntervalSize = 0;
  size_t maxHitSize      = 0;

  for (const char *volumeType : {"amr",
                                 "structuredRegular",
                                 "structuredSpherical",
                                 "particle",
                                 "unstructured",
                                 "vdb"}) {
    VKLVolume volume = vklNewVolume(volumeType);
    VKLSampler sampler = vklNewSampler(volume);
    vklCommit(sampler);
    maxIntervalSize =
        std::max<size_t>(maxIntervalSize, vklGetIntervalIteratorSize(sampler));
    maxHitSize = std::max<size_t>(maxHitSize, vklGetHitIteratorSize(sampler));
    vklRelease(sampler);
    vklRelease(volume);
  }

  REQUIRE(maxIntervalSize == INTERVAL_MACRO);
  REQUIRE(maxHitSize == HIT_MACRO);
}

TEST_CASE("Max iterator size", "[simd_conformance]")
{
  vklLoadModule("ispc_driver");

  for (int W: { 4, 8, 16 })
  {
    std::ostringstream os;
    os << "ispc_" << W;
    VKLDriver driver = vklNewDriver(os.str().c_str());
    if (driver)
    {
      vklCommitDriver(driver);
      vklSetCurrentDriver(driver);
      int nativeSIMDWidth = vklGetNativeSIMDWidth();

      if (nativeSIMDWidth == 4) {
#if VKL_TARGET_WIDTH_ENABLED_4
        SECTION("4-wide")
        {
          max_iterator_size_conformance_test<4,
                            VKL_MAX_INTERVAL_ITERATOR_SIZE_4,
                            VKL_MAX_HIT_ITERATOR_SIZE_4>();
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for driver build configuration");
#endif
      }

      else if (nativeSIMDWidth == 8) {
#if VKL_TARGET_WIDTH_ENABLED_8
        SECTION("8-wide")
        {
          max_iterator_size_conformance_test<8,
                            VKL_MAX_INTERVAL_ITERATOR_SIZE_8,
                            VKL_MAX_HIT_ITERATOR_SIZE_8>();
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for driver build configuration");
#endif
      }

      else if (nativeSIMDWidth == 16) {
#if VKL_TARGET_WIDTH_ENABLED_16
        SECTION("16-wide")
        {
          max_iterator_size_conformance_test<16,
                            VKL_MAX_INTERVAL_ITERATOR_SIZE_16,
                            VKL_MAX_HIT_ITERATOR_SIZE_16>();
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for driver build configuration");
#endif
      }

      else {
        throw std::runtime_error("unsupported native SIMD width for tests");
      }
    }
    else // (driver) 
    {
      WARN("skipping SIMD conformance tests on unsupported width " << W);
    }
  }

}

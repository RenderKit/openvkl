// Copyright 2019-2021 Intel Corporation
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
void device_native_simd_width_conformance_test(VKLDevice device)
{
  INFO("width = " << W);
  REQUIRE(vklGetNativeSIMDWidth(device) == W);
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

  vklLoadModule("cpu_device");

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);

  int nativeSIMDWidth = vklGetNativeSIMDWidth(device);

  WARN("only performing ISPC-side SIMD conformance tests for native width: "
       << nativeSIMDWidth);

  if (nativeSIMDWidth == 4) {
#if VKL_TARGET_WIDTH_ENABLED_4
    SECTION("native (4-wide)")
    {
      device_native_simd_width_conformance_test<4>(device);
      vrange1fn_conformance_test<4>();
      vvec3fn_conformance_test<4>();
      vVKLIntervalN_conformance_test<4>();
      vVKLHitN_conformance_test<4>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for device build configuration");
#endif
  }

  else if (nativeSIMDWidth == 8) {
#if VKL_TARGET_WIDTH_ENABLED_8
    SECTION("native (8-wide)")
    {
      device_native_simd_width_conformance_test<8>(device);
      vrange1fn_conformance_test<8>();
      vvec3fn_conformance_test<8>();
      vVKLIntervalN_conformance_test<8>();
      vVKLHitN_conformance_test<8>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for device build configuration");
#endif
  }

  else if (nativeSIMDWidth == 16) {
#if VKL_TARGET_WIDTH_ENABLED_16
    SECTION("native (16-wide)")
    {
      device_native_simd_width_conformance_test<16>(device);
      vrange1fn_conformance_test<16>();
      vvec3fn_conformance_test<16>();
      vVKLIntervalN_conformance_test<16>();
      vVKLHitN_conformance_test<16>();
    }
#else
    throw std::runtime_error(
        "illegal native SIMD width for device build configuration");
#endif
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }

  vklReleaseDevice(device);
}

template <int W, int INTERVAL_MACRO, int HIT_MACRO>
void max_iterator_size_conformance_test(VKLDevice device)
{
  size_t maxIntervalSize = 0;
  size_t maxHitSize      = 0;

  for (const char *volumeType : {"amr",
                                 "structuredRegular",
                                 "structuredSpherical",
                                 "particle",
                                 "unstructured",
                                 "vdb"}) {
    VKLVolume volume   = vklNewVolume(device, volumeType);
    VKLSampler sampler = vklNewSampler(volume);
    vklCommit(sampler);
    VKLIntervalIteratorContext intervalContext =
        vklNewIntervalIteratorContext(sampler);
    VKLHitIteratorContext hitContext = vklNewHitIteratorContext(sampler);
    maxIntervalSize = std::max<size_t>(
        maxIntervalSize, vklGetIntervalIteratorSize(intervalContext));
    maxHitSize =
        std::max<size_t>(maxHitSize, vklGetHitIteratorSize(hitContext));
    vklRelease(hitContext);
    vklRelease(intervalContext);
    vklRelease(sampler);
    vklRelease(volume);
  }

  REQUIRE(maxIntervalSize == INTERVAL_MACRO);
  REQUIRE(maxHitSize == HIT_MACRO);
}

TEST_CASE("Max iterator size", "[simd_conformance]")
{
  vklLoadModule("cpu_device");

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);

  // the native width for the default device is the maximum we can instantiate
  int maximumSIMDWidth = vklGetNativeSIMDWidth(device);

  vklReleaseDevice(device);

  for (int W : {4, 8, 16}) {
    if (W > maximumSIMDWidth) {
      WARN("skipping max iterator size tests on unsupported device width "
           << W << " (maximum supported width on this system / build is "
           << maximumSIMDWidth << ")");
      continue;
    }

    std::ostringstream os;
    os << "cpu_" << W;
    device = vklNewDevice(os.str().c_str());
    if (device) {
      vklCommitDevice(device);
      int nativeSIMDWidth = vklGetNativeSIMDWidth(device);

      REQUIRE(nativeSIMDWidth == W);

      if (nativeSIMDWidth == 4) {
#if VKL_TARGET_WIDTH_ENABLED_4
        SECTION("4-wide")
        {
          max_iterator_size_conformance_test<4,
                                             VKL_MAX_INTERVAL_ITERATOR_SIZE_4,
                                             VKL_MAX_HIT_ITERATOR_SIZE_4>(
              device);
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for device build configuration");
#endif
      }

      else if (nativeSIMDWidth == 8) {
#if VKL_TARGET_WIDTH_ENABLED_8
        SECTION("8-wide")
        {
          max_iterator_size_conformance_test<8,
                                             VKL_MAX_INTERVAL_ITERATOR_SIZE_8,
                                             VKL_MAX_HIT_ITERATOR_SIZE_8>(
              device);
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for device build configuration");
#endif
      }

      else if (nativeSIMDWidth == 16) {
#if VKL_TARGET_WIDTH_ENABLED_16
        SECTION("16-wide")
        {
          max_iterator_size_conformance_test<16,
                                             VKL_MAX_INTERVAL_ITERATOR_SIZE_16,
                                             VKL_MAX_HIT_ITERATOR_SIZE_16>(
              device);
        }
#else
        throw std::runtime_error(
            "illegal native SIMD width for device build configuration");
#endif
      }

      else {
        throw std::runtime_error("unsupported native SIMD width for tests");
      }

      vklReleaseDevice(device);
    } else  // (device)
    {
      WARN(
          "cannot run max iterator size tests on unavailable (not compiled) "
          "device width "
          << W);
    }
  }
}

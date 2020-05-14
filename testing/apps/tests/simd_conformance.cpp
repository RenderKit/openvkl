// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "../common/simd.h"
#include "openvkl/common.h"
#include "openvkl/drivers/ispc/iterator/GridAcceleratorIterator.h"
#include "openvkl_testing.h"
#include "simd_conformance_ispc.h"

using namespace ospcommon;
using namespace openvkl::testing;
using namespace openvkl;

template <int W>
struct vklPublicWideTypes
{
  using vkl_vvec3fW   = void;
  using vkl_vrange1fW = void;
};

template <>
struct vklPublicWideTypes<1>
{
  using vkl_vvec3fW   = vkl_vec3f;
  using vkl_vrange1fW = vkl_range1f;
};

template <>
struct vklPublicWideTypes<4>
{
  using vkl_vvec3fW   = vkl_vvec3f4;
  using vkl_vrange1fW = vkl_vrange1f4;
};

template <>
struct vklPublicWideTypes<8>
{
  using vkl_vvec3fW   = vkl_vvec3f8;
  using vkl_vrange1fW = vkl_vrange1f8;
};

template <>
struct vklPublicWideTypes<16>
{
  using vkl_vvec3fW   = vkl_vvec3f16;
  using vkl_vrange1fW = vkl_vrange1f16;
};

template <int W>
void public_wide_types_conformance_test()
{
  INFO("width = " << W);

  using vkl_vvec3fW   = typename vklPublicWideTypes<W>::vkl_vvec3fW;
  using vkl_vrange1fW = typename vklPublicWideTypes<W>::vkl_vrange1fW;

  REQUIRE(sizeof(vvec3fn<W>) == sizeof(vkl_vvec3fW));
  REQUIRE(alignof(vvec3fn<W>) == alignof(vkl_vvec3fW));

  REQUIRE(sizeof(vrange1fn<W>) == sizeof(vkl_vrange1fW));
  REQUIRE(alignof(vrange1fn<W>) == alignof(vkl_vrange1fW));
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
void vVKLIntervalIteratorN_conformance_test()
{
  INFO("width = " << W
                  << ", alignment = " << alignof(vVKLIntervalIteratorN<W>));

  REQUIRE(sizeof(vVKLIntervalIteratorN<W>) ==
          ispc::sizeofVaryingVKLIntervalIterator());
  void *ptr = ispc::newVaryingVKLIntervalIterator();
  REQUIRE(is_aligned_for_type<vVKLIntervalIteratorN<W>>(ptr));
  ispc::delete_uniform(ptr);

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
  void *ptr = ispc::newVaryingVKLHitIterator();
  REQUIRE(is_aligned_for_type<vVKLHitIteratorN<W>>(ptr));
  ispc::delete_uniform(ptr);

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

template <int W>
void GridAcceleratorIterator_conformance_test()
{
  // uniform GridAcceleratorIterator
  int ispcSize = ispc::sizeofGridAcceleratorIteratorU();
  REQUIRE(ispcSize ==
          openvkl::ispc_driver::GridAcceleratorIteratorU<W>::ispcStorageSize);

  void *ptr = ispc::newGridAcceleratorIteratorU();
  REQUIRE(
      is_aligned_for_type<openvkl::ispc_driver::GridAcceleratorIteratorU<W>>(
          ptr));
  ispc::delete_uniform(ptr);

  REQUIRE(sizeof(openvkl::ispc_driver::GridAcceleratorIteratorU<W>) <=
          iterator_internal_state_size_for_width(1));

  // varying GridAcceleratorIterator
  ispcSize = ispc::sizeofGridAcceleratorIteratorV();
  REQUIRE(ispcSize ==
          openvkl::ispc_driver::GridAcceleratorIteratorV<W>::ispcStorageSize);

  ptr = ispc::newGridAcceleratorIteratorV();
  REQUIRE(
      is_aligned_for_type<openvkl::ispc_driver::GridAcceleratorIteratorV<W>>(
          ptr));
  ispc::delete_uniform(ptr);

  REQUIRE(sizeof(openvkl::ispc_driver::GridAcceleratorIteratorV<W>) <=
          iterator_internal_state_size_for_width(W));
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
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else if (nativeSIMDWidth == 8) {
#if VKL_TARGET_WIDTH_ENABLED_8
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
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else if (nativeSIMDWidth == 16) {
#if VKL_TARGET_WIDTH_ENABLED_16
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
#else
    throw std::runtime_error(
        "illegal native SIMD width for driver build configuration");
#endif
  }

  else {
    throw std::runtime_error("unsupported native SIMD width for tests");
  }
}

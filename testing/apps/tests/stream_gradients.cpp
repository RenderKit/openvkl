// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "../../external/catch.hpp"
#include "gradient_utility.h"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

TEST_CASE("Stream gradients", "[volume_gradients]")
{
  initializeOpenVKL();

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
  SECTION("structuredRegular")
  {
    auto v = std::make_shared<WaveletStructuredRegularVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL
  SECTION("structuredSpherical")
  {
    auto v = std::make_shared<WaveletStructuredSphericalVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
  SECTION("unstructured")
  {
    auto v = std::make_shared<WaveletUnstructuredProceduralVolume>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }
#endif

#if OPENVKL_DEVICE_CPU_VDB
  SECTION("vdb")
  {
    auto v1 = std::make_shared<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(128), vec3f(0.f), vec3f(1.f), true);
    test_stream_gradients(v1);

    auto v2 = std::make_shared<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(128), vec3f(0.f), vec3f(1.f), false);
    test_stream_gradients(v2);
  }
#endif

  shutdownOpenVKL();
}

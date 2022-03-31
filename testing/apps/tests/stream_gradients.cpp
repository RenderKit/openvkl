// Copyright 2020-2022 Intel Corporation
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

  SECTION("structuredRegular")
  {
    auto v = std::make_shared<WaveletStructuredRegularVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }

  SECTION("structuredSpherical")
  {
    auto v = std::make_shared<WaveletStructuredSphericalVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }

  SECTION("unstructured")
  {
    auto v = std::make_shared<WaveletUnstructuredProceduralVolume>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }

  SECTION("vdb")
  {
    auto v1 = std::make_shared<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(128), vec3f(0.f), vec3f(1.f), true);
    test_stream_gradients(v1);

    auto v2 = std::make_shared<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(128), vec3f(0.f), vec3f(1.f), false);
    test_stream_gradients(v2);
  }

  shutdownOpenVKL();
}

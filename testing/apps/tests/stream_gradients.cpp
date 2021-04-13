// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "gradient_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

TEST_CASE("Stream gradients", "[volume_gradients]")
{
  vklLoadModule("cpu_device");

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);
  vklSetCurrentDevice(device);

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
    auto v = std::make_shared<WaveletVdbVolumeFloat>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_gradients(v);
  }
}

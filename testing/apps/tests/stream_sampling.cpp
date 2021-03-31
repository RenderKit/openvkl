// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

TEST_CASE("Stream sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("AMR")
  {
    auto v = std::make_shared<ProceduralShellsAMRVolume<>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_sampling(v);
  }

  SECTION("structuredRegular")
  {
    auto v = std::make_shared<WaveletStructuredRegularVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_sampling(v);
  }

  SECTION("structuredSpherical")
  {
    auto v = std::make_shared<WaveletStructuredSphericalVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_sampling(v);
  }

  SECTION("unstructured")
  {
    auto v = std::make_shared<WaveletUnstructuredProceduralVolume>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_sampling(v);
  }

  SECTION("VDB")
  {
    auto v = std::make_shared<WaveletVdbVolumeFloat>(
        vec3i(128), vec3f(0.f), vec3f(1.f));
    test_stream_sampling(v);
  }
}

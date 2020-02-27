// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;

template <typename VOLUME_TYPE>
void test_stream_sampling()
{
  auto v =
      ospcommon::make_unique<VOLUME_TYPE>(vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("randomized stream sampling")
  {
    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
    std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

    const int maxN = 1024;

    for (int N = 1; N < maxN; N++) {
      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<float> samples(N);

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeSampleN(vklVolume, N, objectCoordinates.data(), samples.data());

      for (int i = 0; i < N; i++) {
        float sampleTruth = vklComputeSample(vklVolume, &objectCoordinates[i]);

        INFO("sample = " << i + 1 << " / " << N);

        // samples may be NaN if out of bounds of the grid (e.g. for samples in
        // the bounding box of a spherical volume but outside the grid)
        REQUIRE(((sampleTruth == samples[i]) ||
                 (std::isnan(sampleTruth) && std::isnan(samples[i]))));
      }
    }
  }
}

TEST_CASE("Stream sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("AMR")
  {
    test_stream_sampling<ProceduralShellsAMRVolume<>>();
  }

  SECTION("structuredRegular")
  {
    test_stream_sampling<WaveletStructuredRegularVolume<float>>();
  }

  SECTION("structuredSpherical")
  {
    test_stream_sampling<WaveletStructuredSphericalVolume<float>>();
  }

  SECTION("unstructured")
  {
    test_stream_sampling<WaveletUnstructuredProceduralVolume>();
  }

  SECTION("VDB")
  {
    test_stream_sampling<WaveletVdbVolume>();
  }
}

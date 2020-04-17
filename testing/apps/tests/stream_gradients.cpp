// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace ospcommon;
using namespace openvkl::testing;

template <typename VOLUME_TYPE>
void test_stream_gradients()
{
  auto v =
      ospcommon::make_unique<VOLUME_TYPE>(vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume = v->getVKLVolume();

  SECTION("randomized stream gradients")
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
      std::vector<vkl_vec3f> gradients(N);

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeGradientN(
          vklVolume, N, objectCoordinates.data(), gradients.data());

      for (int i = 0; i < N; i++) {
        vkl_vec3f gradientTruth =
            vklComputeGradient(vklVolume, &objectCoordinates[i]);

        INFO("gradient = " << i + 1 << " / " << N);

        // gradients may be NaN if out of bounds of the grid (e.g. for gradients
        // in the bounding box of a spherical volume but outside the grid)
        REQUIRE(
            (((gradientTruth.x == gradients[i].x) ||
              (std::isnan(gradientTruth.x) && std::isnan(gradients[i].x))) &&
             ((gradientTruth.y == gradients[i].y) ||
              (std::isnan(gradientTruth.y) && std::isnan(gradients[i].y))) &&
             ((gradientTruth.z == gradients[i].z) ||
              (std::isnan(gradientTruth.z) && std::isnan(gradients[i].z)))));
      }
    }
  }
}

TEST_CASE("Stream gradients", "[volume_gradients]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("structuredRegular")
  {
    test_stream_gradients<WaveletStructuredRegularVolume<float>>();
  }

  SECTION("structuredSpherical")
  {
    test_stream_gradients<WaveletStructuredSphericalVolume<float>>();
  }

  SECTION("unstructured")
  {
    test_stream_gradients<WaveletUnstructuredProceduralVolume>();
  }
}

// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

void randomized_vectorized_gradients(VKLVolume volume)
{
  vkl_box3f bbox = vklGetBoundingBox(volume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  const int maxWidth = 16;

  std::array<int, 3> nativeWidths{4, 8, 16};

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  for (int width = 1; width < maxWidth; width++) {
    std::vector<vec3f> objectCoordinates(width);
    for (auto &oc : objectCoordinates) {
      oc = vec3f(distX(eng), distY(eng), distZ(eng));
    }

    for (const int &callingWidth : nativeWidths) {
      if (width > callingWidth) {
        continue;
      }

      std::vector<int> valid(callingWidth, 0);
      std::fill(valid.begin(), valid.begin() + width, 1);

      AlignedVector<float> objectCoordinatesSOA =
          AOStoSOA_vec3f(objectCoordinates, callingWidth);

      std::vector<vec3f> gradients;

      if (callingWidth == 4) {
        vkl_vvec3f4 gradients4;
        vklComputeGradient4(valid.data(),
                            sampler,
                            (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                            &gradients4);
        gradients = SOAtoAOS_vvec3f(gradients4);
      } else if (callingWidth == 8) {
        vkl_vvec3f8 gradients8;
        vklComputeGradient8(valid.data(),
                            sampler,
                            (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                            &gradients8);
        gradients = SOAtoAOS_vvec3f(gradients8);
      } else if (callingWidth == 16) {
        vkl_vvec3f16 gradients16;
        vklComputeGradient16(valid.data(),
                             sampler,
                             (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                             &gradients16);
        gradients = SOAtoAOS_vvec3f(gradients16);
      } else {
        throw std::runtime_error("unsupported calling width");
      }

      for (int i = 0; i < width; i++) {
        vkl_vec3f gradientTruth = vklComputeGradient(
            sampler, (const vkl_vec3f *)&objectCoordinates[i]);

        INFO("gradient = " << i + 1 << " / " << width
                           << ", calling width = " << callingWidth);

        REQUIRE(gradientTruth.x == gradients[i].x);
        REQUIRE(gradientTruth.y == gradients[i].y);
        REQUIRE(gradientTruth.z == gradients[i].z);
      }
    }
  }

  vklRelease(sampler);
}

TEST_CASE("Vectorized gradients", "[volume_gradients]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION(
      "randomized vectorized gradients varying calling width and masks: "
      "structured volumes")
  {
    std::unique_ptr<XYZStructuredRegularVolume<float>> v(
        new XYZStructuredRegularVolume<float>(
            vec3i(128), vec3f(0.f), vec3f(1.f)));

    VKLVolume volume = v->getVKLVolume();

    randomized_vectorized_gradients(volume);
  }

  SECTION(
      "randomized vectorized gradients varying calling width and masks: "
      "unstructured volumes")
  {
    std::unique_ptr<XYZUnstructuredProceduralVolume> v(
        new XYZUnstructuredProceduralVolume(
            vec3i(128), vec3f(0.f), vec3f(1.f), VKL_HEXAHEDRON, false));

    VKLVolume volume = v->getVKLVolume();

    randomized_vectorized_gradients(volume);
  }

  SECTION(
      "randomized vectorized gradients varying calling width and masks: "
      "vdb volumes")
  {
    std::unique_ptr<XYZVdbVolume> v(new XYZVdbVolume(
            vec3i(128), vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

    VKLVolume volume = v->getVKLVolume();

    randomized_vectorized_gradients(volume);
  }
}

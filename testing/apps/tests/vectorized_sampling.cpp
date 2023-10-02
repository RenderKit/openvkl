// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

template <typename VOLUME_TYPE>
void test_vectorized_sampling()
{
  auto v =
      rkcommon::make_unique<VOLUME_TYPE>(vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  SECTION("randomized vectorized sampling varying calling width and masks")
  {
    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
    std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

    const int maxWidth = 16;

    std::array<int, 3> nativeWidths{4, 8, 16};

    for (int width = 1; width < maxWidth; width++) {
      std::vector<vec3f> objectCoordinates(width);
      for (auto &oc : objectCoordinates) {
        oc = vec3f(distX(eng), distY(eng), distZ(eng));
      }

      for (auto callingWidth : nativeWidths) {
        if (width > callingWidth) {
          continue;
        }

        std::vector<int> valid(callingWidth, 0);
        std::fill(valid.begin(), valid.begin() + width, 1);

        AlignedVector<float> objectCoordinatesSOA =
            AOStoSOA_vec3f(objectCoordinates, callingWidth);

        float samples[16];

        if (callingWidth == 4) {
          vklComputeSample4(valid.data(),
                            &vklSampler,
                            (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                            samples);
        } else if (callingWidth == 8) {
          vklComputeSample8(valid.data(),
                            &vklSampler,
                            (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                            samples);

        } else if (callingWidth == 16) {
          vklComputeSample16(valid.data(),
                             &vklSampler,
                             (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                             samples);
        } else {
          throw std::runtime_error("unsupported calling width");
        }

        for (int i = 0; i < width; i++) {
          float sampleTruth = vklComputeSample(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates[i]);

          INFO("sample = " << i + 1 << " / " << width
                           << ", calling width = " << callingWidth);
#ifdef __ARM_NEON
          static constexpr float tolerance = 1e-3f;
          REQUIRE(sampleTruth == Approx(samples[i]).margin(tolerance));
#else
          REQUIRE(sampleTruth == Approx(samples[i]));
#endif
        }
      }
    }
  }
  vklRelease(vklSampler);
}

TEST_CASE("Vectorized sampling", "[volume_sampling]")
{
  initializeOpenVKL();

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
  SECTION("structured")
  {
    test_vectorized_sampling<WaveletStructuredRegularVolume<float>>();
  }
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
  SECTION("unstructured")
  {
    test_vectorized_sampling<WaveletUnstructuredProceduralVolume>();
  }
#endif

  shutdownOpenVKL();
}

// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

void scalar_hit_iteration(VKLVolume volume,
                          const std::vector<float> &isoValues,
                          const std::vector<float> &expectedTValues,
                          const vkl_vec3f &origin = vkl_vec3f{0.5f, 0.5f, -1.f},
                          const vkl_vec3f &direction = vkl_vec3f{0.f, 0.f, 1.f})
{
  vkl_range1f tRange{0.f, inf};
  const float time = 0.f;

  VKLValueSelector valueSelector = vklNewValueSelector(volume);
  vklValueSelectorSetValues(valueSelector, isoValues.size(), isoValues.data());
  vklCommit(valueSelector);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  std::vector<char> buffer(vklGetHitIteratorSize(sampler));
  VKLHitIterator iterator = vklInitHitIterator(sampler,
                                               &origin,
                                               &direction,
                                               &tRange,
                                               time,
                                               valueSelector,
                                               buffer.data());

  VKLHit hit;

  int hitCount = 0;

  while (vklIterateHit(iterator, &hit)) {
    INFO("hit t = " << hit.t << ", sample = " << hit.sample);

    REQUIRE(hit.t == Approx(expectedTValues[hitCount]).margin(1e-3f));
    REQUIRE(hit.sample == isoValues[hitCount]);

    hitCount++;
  }

  REQUIRE(hitCount == isoValues.size());

  vklRelease(valueSelector);
  vklRelease(sampler);
}

TEST_CASE("Hit iterator", "[hit_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  // default isovalues
  std::vector<float> defaultIsoValues;
  std::vector<float> defaultExpectedTValues;

  for (float f = 0.1f; f < 1.f; f += 0.1f) {
    defaultIsoValues.push_back(f);
    defaultExpectedTValues.push_back(f + 1.f);
  }

  SECTION("scalar hit iteration")
  {
    SECTION("structured volumes")
    {
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

      VKLVolume vklVolume = v->getVKLVolume();

      scalar_hit_iteration(vklVolume, defaultIsoValues, defaultExpectedTValues);
    }

    SECTION(
        "structured volumes: isovalues at grid accelerator macrocell "
        "boundaries")
    {
      // macrocells are currently 16**3
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

      VKLVolume vklVolume = v->getVKLVolume();

      std::vector<float> macroCellBoundaries;
      std::vector<float> macroCellTValues;

      for (int i = 0; i < 128; i += 16) {
        macroCellBoundaries.push_back(float(i));
        macroCellTValues.push_back(float(i) + 1.f);
      }

      scalar_hit_iteration(vklVolume, macroCellBoundaries, macroCellTValues);
    }

    SECTION("structured volumes: single voxel layer edge case")
    {
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(vec3i(17, 17, 17), vec3f(0.f), vec3f(1.f) / vec3f(16.f)));

      VKLVolume vklVolume = v->getVKLVolume();
      // We're tracing from the back, so we'll hit the isovalues in reverse order
      std::vector<float> reversedIsovalues = defaultIsoValues;
      std::reverse(reversedIsovalues.begin(), reversedIsovalues.end());

      scalar_hit_iteration(vklVolume,
                           reversedIsovalues,
                           defaultExpectedTValues,
                           vkl_vec3f{0.5f, 0.5f, 2.f},
                           //vkl_vec3f{8.f, 8.f, 18.f},
                           vkl_vec3f{0.f, 0.f, -1.f});
    }

    SECTION("unstructured volumes")
    {
      std::unique_ptr<ZUnstructuredProceduralVolume> v(
          new ZUnstructuredProceduralVolume(
              dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));

      VKLVolume vklVolume = v->getVKLVolume();

      scalar_hit_iteration(vklVolume, defaultIsoValues, defaultExpectedTValues);
    }
  }
}

// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "../common/Traits.h"
#include "../common/simd.h"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl;
using namespace openvkl::testing;

void scalar_hit_epsilons(std::shared_ptr<TestingVolume> testingVolume,
                         std::vector<float> isovalues,
                         const vkl_vec3f &origin = vkl_vec3f{0.5f, 0.5f, -1.f},
                         const vkl_vec3f &direction = vkl_vec3f{0.f, 0.f, 1.f})
{
  const vkl_range1f tRange{0.f, inf};
  const float time = 0.f;

  VKLVolume volume   = testingVolume->getVKLVolume();
  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLValueSelector valueSelector = vklNewValueSelector(volume);
  vklValueSelectorSetValues(valueSelector, isovalues.size(), isovalues.data());
  vklCommit(valueSelector);

  std::vector<char> buffer(vklGetHitIteratorSize(sampler));
  VKLHitIterator iterator = vklInitHitIterator(sampler,
                                               &origin,
                                               &direction,
                                               &tRange,
                                               time,
                                               valueSelector,
                                               buffer.data());

  VKLHit hit;
  hit.epsilon = 0.f;

  size_t hitCount = 0;

  while (vklIterateHit(iterator, &hit)) {
    INFO("hit t = " << hit.t << ", sample = " << hit.sample
                    << ", epsilon = " << hit.epsilon);

    REQUIRE(hit.epsilon > 0.f);

    hit.epsilon = 0.f;

    hitCount++;
  }

  REQUIRE(hitCount > 0);

  vklRelease(sampler);
  vklRelease(valueSelector);
}

template <int W>
void vector_hit_epsilons(std::shared_ptr<TestingVolume> testingVolume,
                         std::vector<float> isovalues,
                         const vkl_vec3f &origin = vkl_vec3f{0.5f, 0.5f, -1.f},
                         const vkl_vec3f &direction = vkl_vec3f{0.f, 0.f, 1.f})
{
  using VKLHitIteratorW       = typename vklPublicWideTypes<W>::VKLHitIteratorW;
  using VKLHitW               = typename vklPublicWideTypes<W>::VKLHitW;
  auto vklGetHitIteratorSizeW = vklPublicWideTypes<W>().vklGetHitIteratorSizeW;
  auto vklInitHitIteratorW    = vklPublicWideTypes<W>().vklInitHitIteratorW;
  auto vklIterateHitW         = vklPublicWideTypes<W>().vklIterateHitW;

  using vkl_vvec3fW   = typename vklPublicWideTypes<W>::vkl_vvec3fW;
  using vkl_vrange1fW = typename vklPublicWideTypes<W>::vkl_vrange1fW;

  vkl_vrange1fW tRanges;
  vkl_vvec3fW origins;
  vkl_vvec3fW directions;

  for (int i = 0; i < W; i++) {
    tRanges.lower[i] = 0.f;
    tRanges.upper[i] = inf;

    origins.x[i] = origin.x;
    origins.y[i] = origin.y;
    origins.z[i] = origin.z;

    directions.x[i] = direction.x;
    directions.y[i] = direction.y;
    directions.z[i] = direction.z;
  }

  const std::vector<float> times(W, 0.f);

  VKLVolume volume   = testingVolume->getVKLVolume();
  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLValueSelector valueSelector = vklNewValueSelector(volume);
  vklValueSelectorSetValues(valueSelector, isovalues.size(), isovalues.data());
  vklCommit(valueSelector);

  std::vector<int> valid(W, 1);

  // disable a lane, so we can also ensure hits are not written by inactive
  // lanes
  valid[1] = 0;

  std::vector<char> buffer(vklGetHitIteratorSizeW(sampler));
  VKLHitIteratorW iterator = vklInitHitIteratorW(valid.data(),
                                                 sampler,
                                                 (vkl_vvec3fW *)&origins,
                                                 (vkl_vvec3fW *)&directions,
                                                 (vkl_vrange1fW *)&tRanges,
                                                 times.data(),
                                                 valueSelector,
                                                 buffer.data());

  VKLHitW hit;

  for (int i = 0; i < W; i++)
    hit.epsilon[i] = 0.f;

  size_t hitCount = 0;

  // all results should be the same (for active lanes), since we have the same
  // initial rays across all lanes
  std::vector<int> result(W, 0);

  vklIterateHitW(valid.data(), iterator, &hit, result.data());

  while (result[0]) {
    INFO("hit t = " << hit.t << ", sample = " << hit.sample
                    << ", epsilon = " << hit.epsilon);

    for (int i = 0; i < W; i++) {
      // all active lanes (those besides i==1) should have epsilon values set
      REQUIRE((i == 1 ? hit.epsilon[i] == 0.f : hit.epsilon[i] > 0.f));

      hit.epsilon[i] = 0.f;
    }

    hitCount++;

    vklIterateHitW(valid.data(), iterator, &hit, result.data());
  }

  REQUIRE(hitCount > 0);

  vklRelease(sampler);
  vklRelease(valueSelector);
}

TEST_CASE("Hit iterator epsilon", "[hit_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const int nativeWidth = vklGetNativeSIMDWidth();

  // for structured-like volume types: a unit cube physical grid [(0,0,0),
  // (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  // default isovalues
  std::vector<float> isovalues;

  for (float f = 0.1f; f < 1.f; f += 0.1f) {
    isovalues.push_back(f);
  }

  std::vector<std::string> sectionNames{"structured volumes",
                                        "unstructured volumes",
                                        "VDB volumes",
                                        "AMR volumes",
                                        "particle volumes"};

  std::vector<std::shared_ptr<TestingVolume>> testingVolumes;

  testingVolumes.push_back(
      std::make_shared<ZProceduralVolume>(dimensions, gridOrigin, gridSpacing));

  testingVolumes.push_back(std::make_shared<ZUnstructuredProceduralVolume>(
      dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));

  testingVolumes.push_back(std::make_shared<ZVdbVolumeFloat>(
      dimensions, gridOrigin, gridSpacing, VKL_FILTER_TRILINEAR));

  testingVolumes.push_back(std::make_shared<ProceduralShellsAMRVolume<>>(
      dimensions, gridOrigin, gridSpacing));

  testingVolumes.push_back(std::make_shared<ProceduralParticleVolume>(1000));

  if (sectionNames.size() != testingVolumes.size()) {
    throw std::runtime_error("test configuration error");
  }

  for (int i = 0; i < sectionNames.size(); i++) {
    SECTION(sectionNames[i])
    {
      SECTION("scalar")
      {
        scalar_hit_epsilons(testingVolumes[i], isovalues);
      }

      std::stringstream ss;
      ss << "vector (" << nativeWidth << "-wide";

      SECTION(ss.str())
      {
        if (nativeWidth == 4) {
          vector_hit_epsilons<4>(testingVolumes[i], isovalues);
        } else if (nativeWidth == 8) {
          vector_hit_epsilons<8>(testingVolumes[i], isovalues);
        } else if (nativeWidth == 16) {
          vector_hit_epsilons<16>(testingVolumes[i], isovalues);
        }
      }
    }
  }
};

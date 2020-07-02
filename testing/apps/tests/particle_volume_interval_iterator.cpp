// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "iterator_utility.h"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

void interval_iteration(size_t numParticles,
                        bool provideWeights,
                        float radiusSupportFactor,
                        float clampMaxCumulativeValue)
{
  auto v =
      rkcommon::make_unique<ProceduralParticleVolume>(numParticles,
                                                      provideWeights,
                                                      radiusSupportFactor,
                                                      clampMaxCumulativeValue);

  VKLVolume volume = v->getVKLVolume();

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  const vkl_box3f bbox = vklGetBoundingBox(volume);

  // iterate in the +z direction, from random points in (x, y) beginning outside
  // the volume's bounding box
  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  const float z = bbox.lower.z - 1.f;

  const vkl_vec3f direction{0.f, 0.f, 1.f};
  const vkl_range1f tRange{0.f, inf};

  const size_t N = 1000;
  size_t totalIntervals = 0;

  for (size_t i = 0; i < N; i++) {
    vkl_vec3f origin{distX(eng), distY(eng), z};

    std::vector<char> buffer(vklGetIntervalIteratorSize(volume));
    VKLIntervalIterator iterator = vklInitIntervalIterator(
        volume, &origin, &direction, &tRange, nullptr, buffer.data());

    VKLInterval interval;

    int intervalCount = 0;

    while (vklIterateInterval(iterator, &interval)) {
      INFO("interval tRange = "
           << interval.tRange.lower << ", " << interval.tRange.upper
           << " valueRange = " << interval.valueRange.lower << ", "
           << interval.valueRange.upper
           << ", nominalDeltaT = " << interval.nominalDeltaT);

      vkl_range1f sampledValueRange = computeIntervalValueRange(
          sampler, origin, direction, interval.tRange);

      INFO("sampled value range = " << sampledValueRange.lower << ", "
                                    << sampledValueRange.upper);

      // the sampled value range should be completely within the returned
      // interval value range
      // Uncertainty is there because it is hard to compute correct value ranges
      // for particle volumes. Compare ParticleVolume.cpp.
      constexpr float uncertainty = 0.05f;
      REQUIRE(sampledValueRange.lower >= interval.valueRange.lower * (1-uncertainty));
      REQUIRE(sampledValueRange.upper <= interval.valueRange.upper * (1+uncertainty));

      intervalCount++;
    }

    totalIntervals += intervalCount;
  }

  // Not all rays hit something, but most of them should.
  REQUIRE(totalIntervals / static_cast<double>(N) > 0.9);

  vklRelease(sampler);
}

TEST_CASE("Particle volume interval iterator", "[interval_iterators]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const size_t numParticles                         = 1000;
  const std::vector<bool> provideWeights            = {true, false};
  const std::vector<float> radiusSupportFactors     = {0.1f, 1.f, 3.f};
  const std::vector<float> clampMaxCumulativeValues = {
      0.f, 0.1f, 1.5f, 3.f, 1e6f};

  for (const auto &pw : provideWeights) {
    for (const auto &rsf : radiusSupportFactors) {
      for (const auto &cmcv : clampMaxCumulativeValues) {
        INFO("provideWeights = " << pw << ", radiusSupportFactor = " << rsf
                                 << ", clampMaxCumulativeValue = " << cmcv);

        interval_iteration(numParticles, pw, rsf, cmcv);
      }
    }
  }
}

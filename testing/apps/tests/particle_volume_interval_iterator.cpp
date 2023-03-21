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

  VKLVolume volume = v->getVKLVolume(getOpenVKLDevice());

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  const unsigned int attributeIndex = 0;

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetInt(intervalContext, "attributeIndex", attributeIndex);

  vklCommit(intervalContext);

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
  const float time      = 0.f;
  size_t totalIntervals = 0;

#if defined(OPENVKL_TESTING_GPU)
  const size_t N = 100;
#else
  const size_t N = 1000;
#endif

  // Allocate buffers (USM shared buffers in GPU case)
  int *intervalCount           = allocate<int>(1);
  VKLInterval *intervalsBuffer = allocate<VKLInterval>(maxNumIntervals);
  char *iteratorBuffer =
      allocate<char>(vklGetIntervalIteratorSize(&intervalContext));

  for (size_t i = 0; i < N; i++) {
    vkl_vec3f origin{distX(eng), distY(eng), z};

    *intervalCount = 0;

    auto testIteratorFunc = [=]() {
      VKLIntervalIterator intervalIterator =
          vklInitIntervalIterator(&intervalContext,
                                  &origin,
                                  &direction,
                                  &tRange,
                                  time,
                                  (void *)iteratorBuffer);

      VKLInterval interval;
      while (vklIterateInterval(intervalIterator, &interval)) {
        intervalsBuffer[*intervalCount] = interval;
        *intervalCount += 1;
        if (*intervalCount >= maxNumIntervals) {
          break;
        }
      }
    };

    executeTestFunction(testIteratorFunc);

    if (*intervalCount >= maxNumIntervals) {
      WARN("Interval iterations reached max number of intervals: "
           << maxNumIntervals);
    }

    for (size_t y = 0; y < *intervalCount; y++) {
      VKLInterval interval = intervalsBuffer[y];
      INFO("interval tRange = "
           << interval.tRange.lower << ", " << interval.tRange.upper
           << " valueRange = " << interval.valueRange.lower << ", "
           << interval.valueRange.upper
           << ", nominalDeltaT = " << interval.nominalDeltaT);

      vkl_range1f sampledValueRange = computeIntervalValueRange(
          sampler, attributeIndex, origin, direction, interval.tRange);

      INFO("sampled value range = " << sampledValueRange.lower << ", "
                                    << sampledValueRange.upper);

      // the sampled value range should be completely within the returned
      // interval value range
      // Uncertainty is there because it is hard to compute correct value ranges
      // for particle volumes. Compare ParticleVolume.cpp.
      constexpr float uncertainty = 0.05f;
      REQUIRE(sampledValueRange.lower >=
              interval.valueRange.lower * (1 - uncertainty));
      REQUIRE(sampledValueRange.upper <=
              interval.valueRange.upper * (1 + uncertainty));
    }
    totalIntervals += *intervalCount;
  }

  // Dealocate buffers (USM shared buffers in GPU case)
  deallocate(iteratorBuffer);
  deallocate(intervalsBuffer);
  deallocate(intervalCount);

  // Not all rays hit something, but most of them should.
  REQUIRE(totalIntervals / static_cast<double>(N) > 0.9);

  vklRelease(intervalContext);
  vklRelease(sampler);
}

#if OPENVKL_DEVICE_CPU_PARTICLE || defined(OPENVKL_TESTING_GPU)
TEST_CASE("Particle volume interval iterator", "[interval_iterators]")
{
  initializeOpenVKL();

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

  shutdownOpenVKL();
}
#endif

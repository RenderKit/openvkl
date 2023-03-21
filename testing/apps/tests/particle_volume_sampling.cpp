// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

void sampling_at_particle_centers(size_t numParticles,
                                  bool provideWeights,
                                  float radiusSupportFactor,
                                  float clampMaxCumulativeValue)
{
  auto v =
      rkcommon::make_unique<ProceduralParticleVolume>(numParticles,
                                                      provideWeights,
                                                      radiusSupportFactor,
                                                      clampMaxCumulativeValue);

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  const std::vector<vec4f> particles = v->getParticles();

  for (size_t i = 0; i < particles.size(); i++) {
    const vec4f &p = particles[i];

    const vec3f p3(p.x, p.y, p.z);

    INFO("particle i = " << i << ", p = " << p.x << " " << p.y << " " << p.z
                         << ", radius = " << p.w)

    float referenceValue = v->computeProceduralValue(p3);

    INFO("reference = " << referenceValue)

    test_scalar_and_vector_sampling(
        vklSampler, vec3f(p.x, p.y, p.z), referenceValue, 1e-6f);
  }

  vklRelease(vklSampler);
}

void sampling_at_random_points(size_t numParticles,
                               bool provideWeights,
                               float radiusSupportFactor,
                               float clampMaxCumulativeValue)
{
  auto v =
      rkcommon::make_unique<ProceduralParticleVolume>(numParticles,
                                                      provideWeights,
                                                      radiusSupportFactor,
                                                      clampMaxCumulativeValue);

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::mt19937 eng;

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

#if defined(OPENVKL_TESTING_GPU)
  const size_t N = 200;
#else
  const size_t N = 10000;
#endif

  for (size_t i = 0; i < N; i++) {
    const vec3f objectCoordinates(distX(eng), distY(eng), distZ(eng));

    float referenceValue = v->computeProceduralValue(objectCoordinates);

    test_scalar_and_vector_sampling(
        vklSampler, objectCoordinates, referenceValue, 1e-6f);
  }

  vklRelease(vklSampler);
}

#if OPENVKL_DEVICE_CPU_PARTICLE || defined(OPENVKL_TESTING_GPU)
TEST_CASE("Particle volume sampling", "[volume_sampling]")
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

        sampling_at_particle_centers(numParticles, pw, rsf, cmcv);
        sampling_at_random_points(numParticles, pw, rsf, cmcv);
      }
    }
  }

  shutdownOpenVKL();
}
#endif

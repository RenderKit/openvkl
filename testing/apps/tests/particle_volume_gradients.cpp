// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

void gradients_at_particle_centers(size_t numParticles,
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

    const vkl_vec3f vklGradient =
        vklComputeGradient(vklSampler, (const vkl_vec3f *)&p3);

    const vec3f referenceGradient = v->computeProceduralGradient(p3);

    const float margin = 0.2f;
    CHECK(vklGradient.x == Approx(referenceGradient.x).margin(margin));
    CHECK(vklGradient.y == Approx(referenceGradient.y).margin(margin));
    CHECK(vklGradient.z == Approx(referenceGradient.z).margin(margin));
  }

  vklRelease(vklSampler);
}

TEST_CASE("Particle volume gradients", "[volume_gradients]")
{
  initializeOpenVKL();

  const size_t numParticles                         = 1000;
  const std::vector<bool> provideWeights            = {true, false};
  const std::vector<float> radiusSupportFactors     = {0.1f, 1.f, 3.f};
  const std::vector<float> clampMaxCumulativeValues = {
      0.f};  // gradients do not respect clampMaxCumulativeValue

  for (const auto &pw : provideWeights) {
    for (const auto &rsf : radiusSupportFactors) {
      for (const auto &cmcv : clampMaxCumulativeValues) {
        INFO("provideWeights = " << pw << ", radiusSupportFactor = " << rsf
                                 << ", clampMaxCumulativeValue = " << cmcv);

        gradients_at_particle_centers(numParticles, pw, rsf, cmcv);
      }
    }
  }

  shutdownOpenVKL();
}

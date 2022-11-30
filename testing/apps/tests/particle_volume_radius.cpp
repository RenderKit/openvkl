// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

// we need at least one particle with radius > 0; we'll place it at the origin,
// with this radius.
static float originParticleRadius = 1e-3f;

// verifies sampling at particle center yields the particle weight
static void sampling_at_particle_centers(VKLVolume vklVolume,
                                         const std::vector<vec4f> particles)
{
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

  for (size_t i = 0; i < particles.size(); i++) {
    const vec4f &p = particles[i];

    const vkl_vec3f p3{p.x, p.y, p.z};

    INFO("particle i = " << i << ", p = " << p.x << " " << p.y << " " << p.z
                         << ", radius = " << p.w)

    const bool zeroRadiusParticle = p.w <= 0.f;

    // skip testing any zero-radius particles near the origin (non-zero radius)
    // particle; this will be random and rare
    if (zeroRadiusParticle &&
        length(vec3f(p) - vec3f(0.f)) <= originParticleRadius) {
      continue;
    }

    // if the particle radius is zero, then the sampled results should be zero
    const float referenceValue = zeroRadiusParticle ? 0.f : 1.f;

    const float sampledValue = vklComputeSample(&vklSampler, &p3);
    INFO("sampled = " << sampledValue)

    REQUIRE(referenceValue == sampledValue);
  }

  vklRelease2(vklSampler);
}

#if OPENVKL_DEVICE_CPU_PARTICLE
TEST_CASE("Particle volume radius", "[volume_sampling]")
{
  initializeOpenVKL();

  const size_t numParticles = 100;

  // particles will be seeded within these bounds
  box3f bounds = box3f(-1.f, 1.f);

  int32_t randomSeed = 0;

  // create random number distributions for point centers
  std::mt19937 gen(randomSeed);

  std::uniform_real_distribution<float> centerDistribution_x(bounds.lower.x,
                                                             bounds.upper.x);
  std::uniform_real_distribution<float> centerDistribution_y(bounds.lower.y,
                                                             bounds.upper.y);
  std::uniform_real_distribution<float> centerDistribution_z(bounds.lower.z,
                                                             bounds.upper.z);

  // populate the particles
  std::vector<vec4f> particles(
      numParticles);  // position (x, y, z) and radius (w)

  // random particle positions, zero radius
  for (int i = 0; i < numParticles - 1; i++) {
    auto &p = particles[i];
    p.x     = centerDistribution_x(gen);
    p.y     = centerDistribution_y(gen);
    p.z     = centerDistribution_z(gen);
    p.w     = 0.f;
  }

  // need at least one particle with > 0 radius for a valid volume
  particles.back().x = 0.f;
  particles.back().y = 0.f;
  particles.back().z = 0.f;
  particles.back().w = originParticleRadius;

  VKLVolume volume = vklNewVolume(getOpenVKLDevice(), "particle");

  VKLData positionsData = vklNewData(getOpenVKLDevice(),
                                     numParticles,
                                     VKL_VEC3F,
                                     particles.data(),
                                     VKL_DATA_SHARED_BUFFER,
                                     sizeof(vec4f));
  vklSetData2(volume, "particle.position", positionsData);
  vklRelease(positionsData);

  VKLData radiiData = vklNewData(getOpenVKLDevice(),
                                 numParticles,
                                 VKL_FLOAT,
                                 &(particles.data()[0].w),
                                 VKL_DATA_SHARED_BUFFER,
                                 sizeof(vec4f));
  vklSetData2(volume, "particle.radius", radiiData);
  vklRelease(radiiData);

  const float radiusSupportFactor = 3.f;  // default
  vklSetFloat2(volume, "radiusSupportFactor", radiusSupportFactor);

  vklSetFloat2(volume, "background", 0.f);

  vklCommit2(volume);

  sampling_at_particle_centers(volume, particles);

  // bounding box should not include zero radius particles
  vkl_box3f boundingBox = vklGetBoundingBox(volume);

  REQUIRE((boundingBox.lower.x == -originParticleRadius * radiusSupportFactor &&
           boundingBox.lower.y == -originParticleRadius * radiusSupportFactor &&
           boundingBox.lower.z == -originParticleRadius * radiusSupportFactor));

  REQUIRE((boundingBox.upper.x == originParticleRadius * radiusSupportFactor &&
           boundingBox.upper.y == originParticleRadius * radiusSupportFactor &&
           boundingBox.upper.z == originParticleRadius * radiusSupportFactor));

  vklRelease2(volume);

  shutdownOpenVKL();
}
#endif

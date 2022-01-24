// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

// verifies sampling at particle center yields the particle weight
static void sampling_at_particle_centers(VKLVolume vklVolume,
                                         const std::vector<vec4f> particles)
{
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  for (size_t i = 0; i < particles.size(); i++) {
    const vec4f &p = particles[i];

    const vkl_vec3f p3{p.x, p.y, p.z};

    INFO("particle i = " << i << ", p = " << p.x << " " << p.y << " " << p.z
                         << ", radius = " << p.w)

    const float referenceValue = 1.f;

    const float sampledValue = vklComputeSample(vklSampler, &p3);
    INFO("sampled = " << sampledValue)

    REQUIRE(referenceValue == sampledValue);
  }

  vklRelease(vklSampler);
}

// verifies that sampling outside of the particle center yields zero
static void sampling_at_particle_centers_delta(
    VKLVolume vklVolume, const std::vector<vec4f> particles)
{
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  const float epsilon = 1e-6f;

  for (size_t i = 0; i < particles.size(); i++) {
    const vec4f &p = particles[i];

    const vkl_vec3f p3{p.x + epsilon, p.y + epsilon, p.z + epsilon};

    INFO("particle i = " << i << ", p = " << p3.x << " " << p3.y << " " << p3.z
                         << ", radius = " << p.w)

    const float referenceValue = 0.f;

    const float sampledValue = vklComputeSample(vklSampler, &p3);
    INFO("sampled = " << sampledValue)

    REQUIRE(referenceValue == sampledValue);
  }

  vklRelease(vklSampler);
}

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
  for (int i = 0; i < numParticles; i++) {
    auto &p = particles[i];
    p.x     = centerDistribution_x(gen);
    p.y     = centerDistribution_y(gen);
    p.z     = centerDistribution_z(gen);
    p.w     = 0.f;
  }

  VKLVolume volume = vklNewVolume(getOpenVKLDevice(), "particle");

  VKLData positionsData = vklNewData(getOpenVKLDevice(),
                                     numParticles,
                                     VKL_VEC3F,
                                     particles.data(),
                                     VKL_DATA_SHARED_BUFFER,
                                     sizeof(vec4f));
  vklSetData(volume, "particle.position", positionsData);
  vklRelease(positionsData);

  VKLData radiiData = vklNewData(getOpenVKLDevice(),
                                 numParticles,
                                 VKL_FLOAT,
                                 &(particles.data()[0].w),
                                 VKL_DATA_SHARED_BUFFER,
                                 sizeof(vec4f));
  vklSetData(volume, "particle.radius", radiiData);
  vklRelease(radiiData);

  vklSetFloat(volume, "background", 0.f);

  vklCommit(volume);

  sampling_at_particle_centers(volume, particles);

  sampling_at_particle_centers_delta(volume, particles);

  vklRelease(volume);

  shutdownOpenVKL();
}

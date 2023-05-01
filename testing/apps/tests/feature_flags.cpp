// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

template <typename VOLUME_TYPE>
void test_feature_flags(VOLUME_TYPE &v, VKLFeatureFlags expectedFlags)
{
  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  VKLFeatureFlags featureFlags = vklGetFeatureFlags(vklSampler);

  REQUIRE((featureFlags & expectedFlags) == expectedFlags);

  vklRelease(vklSampler);
}

TEST_CASE("Feature flags")
{
  initializeOpenVKL();

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)

  SECTION("structured")
  {
    auto v = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        vec3i(32), vec3f(0.f), vec3f(1.f));

    test_feature_flags(v, VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME);
  }
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL || defined(OPENVKL_TESTING_GPU)
  SECTION("structuredSpherical")
  {
    auto v = rkcommon::make_unique<WaveletStructuredSphericalVolume<float>>(
        vec3i(32), vec3f(0.f), vec3f(1.f));

    test_feature_flags(v, VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME);
  }
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED || defined(OPENVKL_TESTING_GPU)
  SECTION("unstructured")
  {
    auto v = rkcommon::make_unique<WaveletUnstructuredProceduralVolume>(
        vec3i(32), vec3f(0.f), vec3f(1.f));

    test_feature_flags(v, VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME);
  }
#endif

#if OPENVKL_DEVICE_CPU_PARTICLE || defined(OPENVKL_TESTING_GPU)
  SECTION("Particle")
  {
    auto v = rkcommon::make_unique<ProceduralParticleVolume>(100);

    test_feature_flags(v, VKL_FEATURE_FLAG_PARTICLE_VOLUME);
  }
#endif

#if OPENVKL_DEVICE_CPU_AMR || defined(OPENVKL_TESTING_GPU)
  SECTION("AMR")
  {
    auto v = rkcommon::make_unique<ProceduralShellsAMRVolume<>>(
        vec3i(32), vec3f(0.f), vec3f(1.f));

    test_feature_flags(v, VKL_FEATURE_FLAG_AMR_VOLUME);
  }
#endif

#if OPENVKL_DEVICE_CPU_VDB || defined(OPENVKL_TESTING_GPU)
  SECTION("VDB")
  {
    auto v = rkcommon::make_unique<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(32), vec3f(0.f), vec3f(1.f));

    test_feature_flags(v, VKL_FEATURE_FLAG_VDB_VOLUME);
  }
#endif

  shutdownOpenVKL();
}

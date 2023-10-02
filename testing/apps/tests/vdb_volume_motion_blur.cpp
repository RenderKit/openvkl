// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using openvkl::testing::WaveletVdbVolumeFloat;

#if OPENVKL_DEVICE_CPU_VDB || defined(OPENVKL_TESTING_GPU)
TEST_CASE("VDB volume motion blur", "[volume_sampling]")
{
  initializeOpenVKL();

  const std::vector<TemporalConfig> temporalConfigs{
      TemporalConfig(TemporalConfig::Structured, 2),
      TemporalConfig(TemporalConfig::Structured, 4),
      TemporalConfig(TemporalConfig::Unstructured, 2),
      TemporalConfig(std::vector<float>{0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f})};

  for (auto tc = 0; tc < temporalConfigs.size(); tc++) {
    std::stringstream sectionName;
    sectionName << "temporal config " << tc << " ";
    DYNAMIC_SECTION(sectionName.str())
    {
      WaveletVdbVolumeFloat *volume = nullptr;
      REQUIRE_NOTHROW(volume = new WaveletVdbVolumeFloat(
                          getOpenVKLDevice(),
                          128,
                          vec3f(0.f),
                          vec3f(1.f),
                          false /* repackNodes not supported with temporally
                                   varying volumes */
                          ,
                          temporalConfigs[tc]));
      VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
      VKLSampler vklSampler = vklNewSampler(vklVolume);
      vklCommit(vklSampler);

      REQUIRE_NOTHROW(delete volume);
      vklRelease(vklSampler);
    }
  }

  shutdownOpenVKL();
}
#endif

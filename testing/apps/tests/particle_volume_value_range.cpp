// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"

using namespace rkcommon;
using namespace openvkl::testing;

void computed_vs_api_value_range(size_t numParticles,
                                 bool provideWeights,
                                 float radiusSupportFactor,
                                 float clampMaxCumulativeValue)
{
  auto v =
      rkcommon::make_unique<ProceduralParticleVolume>(numParticles,
                                                      provideWeights,
                                                      radiusSupportFactor,
                                                      clampMaxCumulativeValue);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_range1f apiValueRange = vklGetValueRange(vklVolume);

  range1f computedValueRange = v->getComputedValueRange();

  INFO("api valueRange = " << apiValueRange.lower << " "
                           << apiValueRange.upper);
  INFO("computed valueRange = " << computedValueRange.lower << " "
                                << computedValueRange.upper);

  // our computed value range is an estimate, and similarly the API-returned
  // value range may be conservative
  REQUIRE((computedValueRange.lower >= apiValueRange.lower &&
           computedValueRange.upper <= apiValueRange.upper));
}

TEST_CASE("Particle volume value range", "[volume_value_range]")
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

        computed_vs_api_value_range(numParticles, pw, rsf, cmcv);
      }
    }
  }
}

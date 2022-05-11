// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

void amr_sampling_at_shell_boundaries(vec3i dimensions, vec3i step = vec3i(1))
{
  std::unique_ptr<ProceduralShellsAMRVolume<>> v(
      new ProceduralShellsAMRVolume<>(
          dimensions, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  std::vector<vec3f> offsets;
  offsets.emplace_back(0.0f);
  offsets.emplace_back(0.5f);
  offsets.emplace_back(1.0f);

  offsets.emplace_back(63.0f);
  offsets.emplace_back(63.5f);
  // interpolation in range [64, 66)
  offsets.emplace_back(66.0f);

  offsets.emplace_back(96.0f);
  offsets.emplace_back(97.0f);

  offsets.emplace_back(111.0f);
  // interpolation at [112]
  offsets.emplace_back(114.0f);
  offsets.emplace_back(128.0f);

  for (const vec3f &offset : offsets) {
    const auto offsetWithStep = offset * step;

    vec3f objectCoordinates =
        v->getGridOrigin() + offsetWithStep * v->getGridSpacing();
    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    REQUIRE(
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates) ==
        Approx(v->computeProceduralValue(objectCoordinates)).margin(1e-4f));
  }

  vklRelease(vklSampler);
}

TEST_CASE("AMR volume sampling", "[volume_sampling]")
{
  initializeOpenVKL();

  SECTION("32-bit addressing")
  {
    amr_sampling_at_shell_boundaries(vec3i(256));
  }

  shutdownOpenVKL();
}

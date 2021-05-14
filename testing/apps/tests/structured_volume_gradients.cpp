// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace rkcommon;
using namespace openvkl::testing;

template <typename PROCEDURAL_VOLUME_TYPE>
void scalar_gradients(float tolerance = 0.1f, bool skipBoundaries = false)
{
  const vec3i dimensions(128);
  const float boundingBoxSize = 2.f;

  vec3f gridOrigin;
  vec3f gridSpacing;

  // generate legal grid parameters
  PROCEDURAL_VOLUME_TYPE::generateGridParameters(
      dimensions, boundingBoxSize, gridOrigin, gridSpacing);

  auto v = rkcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions());

  for (const auto &offset : mis) {
    // optionally skip boundary vertices
    if (skipBoundaries &&
        (reduce_min(offset) == 0 || offset.x == dimensions.x - 1 ||
         offset.y == dimensions.y - 1 || offset.z == dimensions.z - 1)) {
      continue;
    }

    const vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offset);

    INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    const vkl_vec3f vklGradient =
        vklComputeGradient(vklSampler, (const vkl_vec3f *)&objectCoordinates);
    const vec3f gradient = (const vec3f &)vklGradient;

    // compare to analytical gradient
    const vec3f proceduralGradient =
        v->computeProceduralGradient(objectCoordinates);

    REQUIRE(gradient.x == Approx(proceduralGradient.x).margin(tolerance));
    REQUIRE(gradient.y == Approx(proceduralGradient.y).margin(tolerance));
    REQUIRE(gradient.z == Approx(proceduralGradient.z).margin(tolerance));
  }

  vklRelease(vklSampler);
}

TEST_CASE("Structured volume gradients", "[volume_gradients]")
{
  initializeOpenVKL();

  SECTION("XYZStructuredRegularVolume<float>")
  {
    scalar_gradients<XYZStructuredRegularVolume<float>>();
  }

  SECTION("WaveletStructuredRegularVolume<float>")
  {
    scalar_gradients<WaveletStructuredRegularVolume<float>>();
  }

  SECTION("XYZStructuredSphericalVolume<float>")
  {
    scalar_gradients<XYZStructuredSphericalVolume<float>>(0.1f, true);
  }

  shutdownOpenVKL();
}

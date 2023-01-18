// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "wrappers.h"

using namespace rkcommon;
using namespace openvkl::testing;

// skipBoundaries is true by default; filtered gradients at boundaries will
// interpolate with background values (i.e. zero), and thus not align with
// procedurally-computed values.
template <typename PROCEDURAL_VOLUME_TYPE>
void scalar_gradients(float tolerance = 0.1f, bool skipBoundaries = true)
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

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

  // For GPU limit number of iterations
#ifdef OPENVKL_TESTING_GPU
  const vec3i step = vec3i(8);
#else
  const vec3i step = vec3i(1);
#endif

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    // optionally skip boundary vertices
    if (skipBoundaries && (reduce_min(offsetWithStep) == 0 ||
                           offsetWithStep.x == dimensions.x - 1 ||
                           offsetWithStep.y == dimensions.y - 1 ||
                           offsetWithStep.z == dimensions.z - 1)) {
      continue;
    }

    const vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    const vkl_vec3f vklGradient = vklComputeGradientWrapper(
        &vklSampler, (const vkl_vec3f *)&objectCoordinates, 0, 0.f);
    const vec3f gradient = (const vec3f &)vklGradient;

    // compare to analytical gradient
    const vec3f proceduralGradient =
        v->computeProceduralGradient(objectCoordinates);

    REQUIRE(gradient.x == Approx(proceduralGradient.x).margin(tolerance));
    REQUIRE(gradient.y == Approx(proceduralGradient.y).margin(tolerance));
    REQUIRE(gradient.z == Approx(proceduralGradient.z).margin(tolerance));
  }

  vklRelease2(vklSampler);
}

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || \
    OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL || defined(OPENVKL_TESTING_GPU)
TEST_CASE("Structured volume gradients", "[volume_gradients]")
{
  initializeOpenVKL();

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
  SECTION("XYZStructuredRegularVolume<float>")
  {
    scalar_gradients<XYZStructuredRegularVolume<float>>();
  }

  SECTION("WaveletStructuredRegularVolume<float>")
  {
    scalar_gradients<WaveletStructuredRegularVolume<float>>();
  }
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL || defined(OPENVKL_TESTING_GPU)
  SECTION("XYZStructuredSphericalVolume<float>")
  {
    scalar_gradients<XYZStructuredSphericalVolume<float>>();
  }
#endif

  shutdownOpenVKL();
}
#endif

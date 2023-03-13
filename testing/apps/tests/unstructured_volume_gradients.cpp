// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "wrappers.h"

using namespace rkcommon;
using namespace openvkl::testing;

void xyz_scalar_gradients(VKLUnstructuredCellType primType)
{
#ifdef OPENVKL_TESTING_GPU
  const auto dimensions = vec3i(32);
#else
  const auto dimensions = vec3i(128);
#endif

  const float boundingBoxSize = float(dimensions.x);

  std::unique_ptr<XYZUnstructuredProceduralVolume> v(
      new XYZUnstructuredProceduralVolume(dimensions,
                                          vec3f(0.f),
                                          boundingBoxSize / vec3f(dimensions),
                                          primType,
                                          false));

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions());

  for (const auto &offset : mis) {
    const vec3f objectCoordinates =
        v->getGridOrigin() + offset * v->getGridSpacing();

    INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    const vkl_vec3f vklGradient = vklComputeGradientWrapper(
        &vklSampler, (const vkl_vec3f *)&objectCoordinates, 0, 0);
    const vec3f gradient = (const vec3f &)vklGradient;

    // compare to analytical gradient
    const vec3f proceduralGradient =
        v->computeProceduralGradient(objectCoordinates);

    // gradients should be ~exact on this volume type, so set a tighter
    // tolerance
    REQUIRE(gradient.x == Approx(proceduralGradient.x).epsilon(1e-4f));
    REQUIRE(gradient.y == Approx(proceduralGradient.y).epsilon(1e-4f));
    REQUIRE(gradient.z == Approx(proceduralGradient.z).epsilon(1e-4f));
  }

  vklRelease(vklSampler);
}

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
TEST_CASE("Unstructured volume gradients", "[volume_gradients]")
{
  initializeOpenVKL();

  SECTION("XYZProceduralVolume")
  {
    xyz_scalar_gradients(VKL_HEXAHEDRON);
  }

  shutdownOpenVKL();
}
#endif

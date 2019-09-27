// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "ospcommon/math/box.h"
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

void xyz_scalar_gradients(VKLUnstructuredCellType primType)
{
  const vec3i dimensions(128);
  const float boundingBoxSize = 128.f;

  std::unique_ptr<XYZUnstructuredProceduralVolume> v(
      new XYZUnstructuredProceduralVolume(dimensions,
                                          vec3f(0.f),
                                          boundingBoxSize / vec3f(dimensions),
                                          primType,
                                          false));

  VKLVolume vklVolume = v->getVKLVolume();

  multidim_index_sequence<3> mis(v->getDimensions());

  for (const auto &offset : mis) {
    const vec3f objectCoordinates =
        v->getGridOrigin() + offset * v->getGridSpacing();

    INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    const vkl_vec3f vklGradient =
        vklComputeGradient(vklVolume, (const vkl_vec3f *)&objectCoordinates);
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
}

TEST_CASE("Unstructured volume gradients", "[volume_gradients]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("XYZProceduralVolume")
  {
    xyz_scalar_gradients(VKL_HEXAHEDRON);
  }
}
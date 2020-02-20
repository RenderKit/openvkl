// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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
#include "ospcommon/utility/multidim_index_sequence.h"

using namespace ospcommon;
using namespace openvkl::testing;

void test_bounding_box(const vec3i &dimensions,
                       const vec3f &gridOrigin,
                       const vec3f &gridSpacing)
{
  auto v = ospcommon::make_unique<WaveletStructuredSphericalVolume<float>>(
      dimensions, gridOrigin, gridSpacing);

  VKLVolume vklVolume      = v->getVKLVolume();
  vkl_box3f vklBoundingBox = vklGetBoundingBox(vklVolume);

  INFO("dimensions = " << dimensions.x << " " << dimensions.y << " "
                       << dimensions.z);
  INFO("gridOrigin = " << gridOrigin.x << " " << gridOrigin.y << " "
                       << gridOrigin.z);
  INFO("gridSpacing = " << gridSpacing.x << " " << gridSpacing.y << " "
                        << gridSpacing.z);

  INFO("VKL bounding box = ("
       << vklBoundingBox.lower.x << ", " << vklBoundingBox.lower.y << ", "
       << vklBoundingBox.lower.z << ") -> (" << vklBoundingBox.upper.x << ", "
       << vklBoundingBox.upper.y << ", " << vklBoundingBox.upper.z << ")");

  const float vklBoundingBoxVolume =
      (vklBoundingBox.upper.x - vklBoundingBox.lower.x) *
      (vklBoundingBox.upper.y - vklBoundingBox.lower.y) *
      (vklBoundingBox.upper.z - vklBoundingBox.lower.z);

  // bounding box better be non-empty
  REQUIRE(vklBoundingBoxVolume > 0.f);

  box3f computedBoundingBox = empty;

  multidim_index_sequence<3> mis(v->getDimensions());

  for (const auto &localCoordinates : mis) {
    vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(localCoordinates);

    computedBoundingBox.extend(objectCoordinates);
  }

  INFO("computed bounding box = ("
       << computedBoundingBox.lower.x << ", " << computedBoundingBox.lower.y
       << ", " << computedBoundingBox.lower.z << ") -> ("
       << computedBoundingBox.upper.x << ", " << computedBoundingBox.upper.y
       << ", " << computedBoundingBox.upper.z << ")");

  // all margins should be >= 0, and represent how much "inside" the computed
  // bounding box is within the VKL returned bounding box
  const vec3f lowerBoundMargin =
      computedBoundingBox.lower - vec3f(vklBoundingBox.lower.x,
                                        vklBoundingBox.lower.y,
                                        vklBoundingBox.lower.z);

  const vec3f upperBoundMargin = vec3f(vklBoundingBox.upper.x,
                                       vklBoundingBox.upper.y,
                                       vklBoundingBox.upper.z) -
                                 computedBoundingBox.upper;

  const float minimumMargin =
      min(reduce_min(lowerBoundMargin), reduce_min(upperBoundMargin));

  // verify computed bounding box is completely within VKL-returned bounding
  // box; allow tolerance due to numerics differences with ISPC trig functions
  REQUIRE(minimumMargin >= -1e-6f);

  // the VKL bounding box shouldn't be too conservative; it should be a tight
  // bound. warn if this is not the case. note, our computed volume bounding box
  // will always be less accurate than the VKL-returned value, especially for
  // small values of `dimensions`.
  const float computedBoundingBoxVolume = computedBoundingBox.size().product();

  const float volumeRatio = computedBoundingBoxVolume / vklBoundingBoxVolume;

  if (volumeRatio < 0.8f) {
    WARN("computed bounding box volume is "
         << volumeRatio << "x the VKL bounding box volume (desired = 1)");
  }
}

TEST_CASE("Structured spherical volume bounding box", "[volume_bounding_box]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  std::vector<int> dimensions = {4, 32};

  std::vector<float> radiusOrigins = {0.f, 1.f};
  std::vector<float> radiusSizes   = {0.1f, 1.f, -1.f};

  std::vector<float> inclinationOrigins = {0.f, 30.f, 90.f};
  std::vector<float> inclinationSizes   = {10.f, 90.f, -90.f};

  std::vector<float> azimuthOrigins = {0.f, 60.f, 180.f};
  std::vector<float> azimuthSizes   = {10.f, 90.f, 180.f, -180.f};

  // multidim_index_sequence is limited to 3D, so use two of them!
  multidim_index_sequence<3> ijks(vec3i(
      radiusOrigins.size(), radiusSizes.size(), inclinationOrigins.size()));
  multidim_index_sequence<3> lmns(vec3i(
      inclinationSizes.size(), azimuthOrigins.size(), azimuthSizes.size()));

  // legal grid ranges
  const range1f legalInclinationRange(0.f, 180.f);
  const range1f legalAzimuthRange(0.f, 360.f);

  for (auto dim : dimensions) {
    for (const auto &ijk : ijks) {
      for (const auto &lmn : lmns) {
        const float radiusOrigin      = radiusOrigins[ijk.x];
        const float radiusSize        = radiusSizes[ijk.y];
        const float inclinationOrigin = inclinationOrigins[ijk.z];

        const float inclinationSize = inclinationSizes[lmn.x];
        const float azimuthOrigin   = azimuthOrigins[lmn.y];
        const float azimuthSize     = azimuthSizes[lmn.z];

        // skip illegal grids
        range1f radiusRange = empty;
        radiusRange.extend(radiusOrigin);
        radiusRange.extend(radiusOrigin + radiusSize);

        range1f inclinationRange;
        inclinationRange.extend(inclinationOrigin);
        inclinationRange.extend(inclinationOrigin + inclinationSize);

        range1f azimuthRange;
        azimuthRange.extend(azimuthOrigin);
        azimuthRange.extend(azimuthOrigin + azimuthSize);

        if (radiusRange.lower < 0.f ||
            inclinationRange.lower < legalInclinationRange.lower ||
            inclinationRange.upper > legalInclinationRange.upper ||
            azimuthRange.lower < legalAzimuthRange.lower ||
            azimuthRange.upper > legalAzimuthRange.upper) {
          continue;
        }

        // specified grids must be within legal ranges for radius, inclination
        // and azimuth. some of the test case grids are exactly at these limits,
        // so we include an epsilon in the computed gridSpacing to avoid
        // exceeding these limits.
        test_bounding_box(vec3i(dim),
                          vec3f(radiusOrigin, inclinationOrigin, azimuthOrigin),
                          (1.f - std::numeric_limits<float>::epsilon()) *
                              vec3f(radiusSize, inclinationSize, azimuthSize) /
                              float(dim - 1));
      }
    }
  }
}

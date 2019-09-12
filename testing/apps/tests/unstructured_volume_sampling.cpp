// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

using namespace ospcommon;
using namespace openvkl::testing;

template <typename volumeType>
void scalar_sampling_test_prim_geometry(VKLUnstructuredCellType primType,
                                        bool cellValued,
                                        bool indexPrefix,
                                        bool precomputedNormals,
                                        bool hexIterative)
{
  std::unique_ptr<volumeType> v(new volumeType(vec3i(1, 1, 1),
                                               vec3f(0, 0, 0),
                                               vec3f(1, 1, 1),
                                               primType,
                                               cellValued,
                                               indexPrefix,
                                               precomputedNormals,
                                               hexIterative));

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> dist(-0.1, 1.1);

  for (int i = 0; i < 1000; i++) {
    vec3f objectCoordinates(dist(eng), dist(eng), dist(eng));
    float sample =
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates);

    bool inside = true;
    if (objectCoordinates.x < 0 || objectCoordinates.x > 1)
      inside = false;
    if (objectCoordinates.y < 0 || objectCoordinates.y > 1)
      inside = false;
    if (objectCoordinates.z < 0 || objectCoordinates.z > 1)
      inside = false;

    switch (primType) {
    case VKL_TETRAHEDRON:
      if (dot(objectCoordinates - vec3f(1, 0, 0), vec3f(1, 1, 1)) > 0)
        inside = false;
      break;
    case VKL_WEDGE:
      if (dot(objectCoordinates - vec3f(1, 0, 0), vec3f(1, 1, 0)) > 0)
        inside = false;
      break;
    case VKL_PYRAMID:
      if (dot(objectCoordinates - vec3f(0, 0, 1), vec3f(1, 0, 1)) > 0)
        inside = false;
      if (dot(objectCoordinates - vec3f(0, 0, 1), vec3f(0, 1, 1)) > 0)
        inside = false;
      break;
    }

    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);
    INFO("inside = " << inside);
    INFO("sample = " << sample);
    CHECK(
        (!inside ? std::isnan(sample) : (sample == Approx(0.5).margin(1e-4f))));
  }
}

TEST_CASE("Unstructured volume sampling", "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  SECTION("hexahedron")
  {
    for (int i = 0; i < 16; i++) {
      bool cellValued         = i & 8;
      bool indexPrefix        = i & 4;
      bool precomputedNormals = i & 2;
      bool hexIterative       = i & 1;
      std::stringstream ss;
      INFO("cellValued = " << cellValued << " indexPrefix = " << indexPrefix
                           << " precomputedNormals = " << precomputedNormals
                           << " hexIterative = " << hexIterative);
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume>(
          VKL_HEXAHEDRON,
          cellValued,
          indexPrefix,
          precomputedNormals,
          hexIterative);
      INFO("64-bit");
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume64>(
          VKL_HEXAHEDRON,
          cellValued,
          indexPrefix,
          precomputedNormals,
          hexIterative);
    }
  }

  SECTION("tetrahedron")
  {
    for (int i = 0; i < 8; i++) {
      bool cellValued         = i & 4;
      bool indexPrefix        = i & 2;
      bool precomputedNormals = i & 1;
      std::stringstream ss;
      INFO("cellValued = " << cellValued << " indexPrefix = " << indexPrefix
                           << " precomputedNormals = " << precomputedNormals);
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume>(
          VKL_TETRAHEDRON, cellValued, indexPrefix, precomputedNormals, false);
      INFO("64-bit");
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume64>(
          VKL_TETRAHEDRON, cellValued, indexPrefix, precomputedNormals, false);
    }
  }

  SECTION("wedge")
  {
    for (int i = 0; i < 8; i++) {
      bool cellValued         = i & 4;
      bool indexPrefix        = i & 2;
      bool precomputedNormals = i & 1;
      std::stringstream ss;
      INFO("cellValued = " << cellValued << " indexPrefix = " << indexPrefix
                           << " precomputedNormals = " << precomputedNormals);
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume>(
          VKL_WEDGE, cellValued, indexPrefix, precomputedNormals, false);
      INFO("64-bit");
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume64>(
          VKL_WEDGE, cellValued, indexPrefix, precomputedNormals, false);
    }
  }

  SECTION("pyramid")
  {
    for (int i = 0; i < 8; i++) {
      bool cellValued         = i & 4;
      bool indexPrefix        = i & 2;
      bool precomputedNormals = i & 1;
      std::stringstream ss;
      INFO("cellValued = " << cellValued << " indexPrefix = " << indexPrefix
                           << " precomputedNormals = " << precomputedNormals);
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume>(
          VKL_PYRAMID, cellValued, indexPrefix, precomputedNormals, false);
      INFO("64-bit");
      scalar_sampling_test_prim_geometry<ConstUnstructuredProceduralVolume64>(
          VKL_PYRAMID, cellValued, indexPrefix, precomputedNormals, false);
    }
  }
}

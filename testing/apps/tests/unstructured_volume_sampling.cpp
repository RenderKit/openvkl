// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "unstructured_volume.h"

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

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> dist(-0.1, 1.1);

  // matches tolerances defined in unstructured implementation, e.g.
  // HEX_OUTSIDE_CELL_TOLERANCE
  constexpr float tolerance = 1e-6f;

  for (int i = 0; i < 1000; i++) {
    vec3f oc(dist(eng), dist(eng), dist(eng));
    float sample = vklComputeSample(&vklSampler, (const vkl_vec3f *)&oc);

    bool inside = true;

    const float objectCoordinateMin = reduce_min(oc);
    const float objectCoordinateMax = reduce_max(oc);

    // positive values indicate we're outside of the cell, and can be used to
    // compare against the above tolerance
    float delta = -1.f;

    // bounding box test, lower bound (0, 0, 0)
    if ((delta = 0.f - objectCoordinateMin) > 0) {
      inside = false;
    }
    // bounding box test, upper bound (1, 1, 1)
    else if ((delta = objectCoordinateMax - 1.f) > 0) {
      inside = false;
    } else {
      switch (primType) {
      case VKL_TETRAHEDRON:
        if ((delta = dot(oc - vec3f(1, 0, 0), normalize(vec3f(1, 1, 1)))) > 0)
          inside = false;
        break;
      case VKL_WEDGE:
        if ((delta = dot(oc - vec3f(1, 0, 0), normalize(vec3f(1, 1, 0)))) > 0)
          inside = false;
        break;
      case VKL_PYRAMID:
        if ((delta = dot(oc - vec3f(0, 0, 1), normalize(vec3f(1, 0, 1)))) > 0)
          inside = false;
        else if ((delta = dot(oc - vec3f(0, 0, 1), normalize(vec3f(0, 1, 1)))) >
                 0)
          inside = false;
        break;
      case VKL_HEXAHEDRON:
        // already handled by bounding box test above
        break;
      }
    }

    INFO("objectCoordinates = " << oc.x << " " << oc.y << " " << oc.z);
    INFO("inside = " << inside);
    INFO("delta = " << delta);
    INFO("sample = " << sample);

    if (delta <= 0.f) {
      REQUIRE((inside && sample == Approx(0.5).margin(1e-4f)));
    } else if (delta > 0.f && delta > tolerance) {
      REQUIRE((!inside && std::isnan(sample)));
    } else {
      // delta > 0 && delta <= tolerance
      WARN("ignoring test failure due to convergence tolerance (delta = "
           << delta << ")");
    }
  }

  vklRelease(vklSampler);
}

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
TEST_CASE("Unstructured volume sampling", "[volume_sampling]")
{
  initializeOpenVKL();

  SECTION("hexahedron")
  {
    scalar_sampling_on_vertices_vs_procedural_values(vec3i(128),
                                                     VKL_HEXAHEDRON);

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
    scalar_sampling_on_vertices_vs_procedural_values(vec3i(128),
                                                     VKL_TETRAHEDRON);

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
    scalar_sampling_on_vertices_vs_procedural_values(vec3i(128), VKL_WEDGE);

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
    scalar_sampling_on_vertices_vs_procedural_values(vec3i(128), VKL_PYRAMID);

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

  shutdownOpenVKL();
}
#endif

// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "unstructured_volume.h"

TEST_CASE("Unstructured volume strides", "[volume_strides]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const vec3i dimensions = vec3i(128);

  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};

  const std::vector<float> strideFactors{1.f, 2.f};

  // use a large step for stride tests here, given the more comprehensive
  // sampling tests elsewhere
  const vec3i step = vec3i(16);

  for (const auto &dcf : dataCreationFlags) {
    for (const auto &strideFactor : strideFactors) {
      std::stringstream sectionName;
      sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                              : "VKL_DATA_SHARED_BUFFER");
      sectionName << ", stride factor: " << strideFactor;

      const size_t byteStride =
          strideFactor == 0.f ? sizeof(float) : strideFactor * sizeof(float);

      INFO("byteStride = " << byteStride);

      // can't have duplicate section names at the same level
      DYNAMIC_SECTION(sectionName.str())
      {
        SECTION("hexahedron")
        {
          scalar_sampling_on_vertices_vs_procedural_values(
              dimensions, VKL_HEXAHEDRON, dcf, byteStride, step);
        }

        SECTION("tetrahedron")
        {
          scalar_sampling_on_vertices_vs_procedural_values(
              dimensions, VKL_TETRAHEDRON, dcf, byteStride, step);
        }

        SECTION("wedge")
        {
          scalar_sampling_on_vertices_vs_procedural_values(
              dimensions, VKL_WEDGE, dcf, byteStride, step);
        }

        SECTION("pyramid")
        {
          scalar_sampling_on_vertices_vs_procedural_values(
              dimensions, VKL_PYRAMID, dcf, byteStride, step);
        }
      }
    }
  }
}

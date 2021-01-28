// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "gradient_utility.h"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

template <typename VOLUME_TYPE>
inline void num_attributes(std::shared_ptr<VOLUME_TYPE> v)
{
  VKLVolume vklVolume = v->getVKLVolume();
  REQUIRE(vklGetNumAttributes(vklVolume) == v->getNumAttributes());
}

TEST_CASE("Structured regular volume multiple attributes",
          "[volume_multi_attributes]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};

  const std::vector<bool> useAOSLayouts{true, false};

  for (const auto &dcf : dataCreationFlags) {
    for (const auto &aos : useAOSLayouts) {
      std::stringstream sectionName;
      sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                              : "VKL_DATA_SHARED_BUFFER");

      sectionName << " ";

      sectionName << (aos == true ? "AOS layout" : "SOA layout");

      DYNAMIC_SECTION(sectionName.str())
      {
        std::shared_ptr<TestingStructuredVolumeMulti> v(
            generateMultiAttributeStructuredRegularVolume(dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          TemporalConfig(),
                                                          dcf,
                                                          aos));

        num_attributes(v);
        sampling_on_vertices_vs_procedural_values_multi(v, 2);
        gradients_on_vertices_vs_procedural_values_multi(v, 2);

        for (unsigned int i = 0; i < v->getNumAttributes(); i++) {
          test_stream_sampling(v, i);
          test_stream_gradients(v, i);
        }

        std::vector<unsigned int> attributeIndices(v->getNumAttributes());
        std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

        test_stream_sampling_multi(v, attributeIndices);
      }
    }
  }
}

// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "gradient_utility.h"
#include "multi_attrib_utility.h"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
TEST_CASE("Structured regular volume multiple attributes",
          "[volume_multi_attributes]")
{
  initializeOpenVKL();

  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};

  const std::vector<bool> useAOSLayouts{true, false};

  const vec3i step = vec3i(4);

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
        sampling_on_vertices_vs_procedural_values_multi(v, step);
        gradients_on_vertices_vs_procedural_values_multi(v, step);

        for (unsigned int i = 0; i < v->getNumAttributes(); i++) {
          computed_vs_api_value_range(v, i);
          test_stream_sampling(v, i);
          test_stream_gradients(v, i);
        }

        std::vector<unsigned int> attributeIndices(v->getNumAttributes());
        std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

        test_stream_sampling_multi(v, attributeIndices);
      }
    }
  }

  shutdownOpenVKL();
}
#endif

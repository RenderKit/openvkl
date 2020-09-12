// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "gradient_utility.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

inline void num_attributes(std::shared_ptr<TestingStructuredVolumeMulti> v)
{
  VKLVolume vklVolume = v->getVKLVolume();
  REQUIRE(vklGetNumAttributes(vklVolume) == v->getNumAttributes());
}

inline void sampling_on_vertices_vs_procedural_values_multi(
    std::shared_ptr<TestingStructuredVolumeMulti> v, vec3i step = vec3i(1))
{
  const float sampleTolerance = 1e-4f;

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    const vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    std::vector<float> proceduralValues;

    for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
      proceduralValues.push_back(
          v->computeProceduralValue(objectCoordinates, a));
    }

    for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
      test_scalar_and_vector_sampling(vklSampler,
                                      objectCoordinates,
                                      proceduralValues[a],
                                      sampleTolerance,
                                      a);
    }

    // std::vector<unsigned int> attributeIndices(v->getNumAttributes());
    // std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

    // test_scalar_and_vector_sampling_multi(vklSampler,
    //                                       objectCoordinates,
    //                                       proceduralValues,
    //                                       sampleTolerance,
    //                                       attributeIndices);
  }

  vklRelease(vklSampler);
}

inline void gradients_on_vertices_vs_procedural_values_multi(
    std::shared_ptr<TestingStructuredVolumeMulti> v, vec3i step = vec3i(1))
{
  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (unsigned int attributeIndex = 0; attributeIndex < v->getNumAttributes();
       attributeIndex++) {
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;

      vec3f objectCoordinates =
          v->transformLocalToObjectCoordinates(offsetWithStep);

      const vec3f proceduralGradient =
          v->computeProceduralGradient(objectCoordinates, attributeIndex);

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      // larger tolerance since gradients are not exact (e.g. computed via
      // finite differences)
      test_scalar_and_vector_gradients(vklSampler,
                                       objectCoordinates,
                                       proceduralGradient,
                                       0.1f,
                                       attributeIndex);
    }
  }

  vklRelease(vklSampler);
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
            generateMultiAttributeStructuredRegularVolume(
                dimensions, gridOrigin, gridSpacing, dcf, aos));

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

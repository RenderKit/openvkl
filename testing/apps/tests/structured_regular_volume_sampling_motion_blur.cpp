// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "rkcommon/utility/random.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

// these tests are only exact for procedural volume fields that vary linearly
// with time!
inline void sampling_on_vertices_vs_procedural_values_motion_blur(
    std::shared_ptr<TestingStructuredVolumeMulti> v, vec3i step = vec3i(1))
{
  if (!v->getTemporalConfig().hasTime()) {
    throw std::runtime_error(
        "these tests only legal for temporally varying volumes");
  }

  const float sampleTolerance = 0.f;

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
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

    for (float time : {0.f, .25f, .5f, .75f, 1.0f}) {
      INFO("time = " << time);
      proceduralValues.clear();
      for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
        proceduralValues.push_back(
            v->computeProceduralValue(objectCoordinates, a, time));
      }

      for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
        INFO("attribute index = " << a);
        test_scalar_and_vector_sampling(vklSampler,
                                        objectCoordinates,
                                        proceduralValues[a],
                                        sampleTolerance,
                                        a,
                                        time);
      }

      std::vector<unsigned int> attributeIndices(v->getNumAttributes());
      std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

      test_scalar_and_vector_sampling_multi(vklSampler,
                                            objectCoordinates,
                                            proceduralValues,
                                            sampleTolerance,
                                            attributeIndices,
                                            time);
    }
  }

  vklRelease(vklSampler);
}

// this tests temporally unstructured with varying time step counts per voxel;
// other tests currently use a constant number of time steps per voxel
inline void sampling_on_vertices_vs_procedural_values_varying_TUV_data()
{
  auto volume = vklNewVolume(getOpenVKLDevice(), "structuredRegular");

  const vec3i dimensions(2);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f);
  vklSetVec3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vklSetVec3f(volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
  vklSetVec3f(
      volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

  std::vector<unsigned int> indices(9, 0);
  std::vector<float> times(20, 0.f);
  std::vector<float> voxels(20, 0.f);

  size_t indexSum = 0;
  for (size_t z = 0; z < 2; z++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        size_t numTimeSamples                 = x == 0 ? 2 : 3;
        size_t index                          = z * 4 + y * 2 + x;
        indices[index]                        = indexSum;
        times[indexSum]                       = 0.f;
        voxels[indexSum]                      = 0.f;
        times[indexSum + numTimeSamples - 1]  = 1.0f;
        voxels[indexSum + numTimeSamples - 1] = x + y + z;
        if (x == 1) {
          times[indexSum + 1]  = 0.5f;
          voxels[indexSum + 1] = 0.5f * (x + y + z);
        }
        indexSum += numTimeSamples;
      }
    }
  }
  indices[8] = indexSum;

  VKLData data =
      vklNewData(getOpenVKLDevice(), voxels.size(), VKL_FLOAT, voxels.data());
  VKLData indicesData =
      vklNewData(getOpenVKLDevice(), indices.size(), VKL_UINT, indices.data());
  VKLData timesData =
      vklNewData(getOpenVKLDevice(), times.size(), VKL_FLOAT, times.data());

  vklSetData(volume, "data", data);
  vklSetData(volume, "temporallyUnstructuredIndices", indicesData);
  vklSetData(volume, "temporallyUnstructuredTimes", timesData);

  vklRelease(data);
  vklRelease(indicesData);
  vklRelease(timesData);

  vklCommit(volume);

  const float sampleTolerance = 0.f;

  VKLSampler vklSampler = vklNewSampler(volume);
  vklCommit(vklSampler);

  multidim_index_sequence<3> mis(dimensions);

  for (float time : {0.f, .25f, .5f, .75f, 1.0f}) {
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset;

      const vec3f objectCoordinates = offsetWithStep;

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);
      INFO("time = " << time);

      test_scalar_and_vector_sampling(
          vklSampler,
          objectCoordinates,
          time *
              (objectCoordinates.x + objectCoordinates.y + objectCoordinates.z),
          sampleTolerance,
          0,
          time);
    }
  }

  vklRelease(vklSampler);
  vklRelease(volume);
}

TEST_CASE("Structured regular volume sampling with motion blur",
          "[volume_sampling]")
{
  initializeOpenVKL();

  SECTION("temporally unstructured with varying time steps per voxel")
  {
    sampling_on_vertices_vs_procedural_values_varying_TUV_data();
  }

  const vec3i dimensions(32);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (32.f - 1.f));

  const std::vector<TemporalConfig> temporalConfigs{
      TemporalConfig(TemporalConfig::Structured, 2),
      TemporalConfig(TemporalConfig::Structured, 4),
      TemporalConfig(TemporalConfig::Unstructured, 2),
      TemporalConfig(std::vector<float>{0.f, 0.15f, 0.3f, 0.65f, 0.9f, 1.0f})};

  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};

  std::random_device rd;
  rkcommon::utility::pcg32_biased_float_distribution dist(rd(), 0, 0.f, 1.f);

  for (auto tc = 0; tc < temporalConfigs.size(); tc++) {
    for (const auto &dcf : dataCreationFlags) {
      std::stringstream sectionName;
      sectionName << "temporal config " << tc << " "
                  << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                              : "VKL_DATA_SHARED_BUFFER");

      DYNAMIC_SECTION(sectionName.str())
      {
        std::shared_ptr<TestingStructuredVolumeMulti> v(
            generateMultiAttributeStructuredRegularVolume(dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          temporalConfigs[tc],
                                                          dcf,
                                                          false));

        VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());
        REQUIRE(vklGetNumAttributes(vklVolume) == v->getNumAttributes());

        sampling_on_vertices_vs_procedural_values_motion_blur(v, 2);

        for (float time : {0.f, dist(), dist(), 1.0f}) {
          for (unsigned int i = 0; i < v->getNumAttributes(); i++) {
            test_stream_sampling(v, i, time);
          }

          std::vector<unsigned int> attributeIndices(v->getNumAttributes());
          std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

          test_stream_sampling_multi(v, attributeIndices, time);
        }
      }
    }
  }

  shutdownOpenVKL();
}

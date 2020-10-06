// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

inline void sampling_on_vertices_vs_procedural_values_varying_TUV_data()
{
  auto volume = vklNewVolume("structuredRegular");

  const vec3i dimensions(2);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f);
  vklSetVec3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vklSetVec3f(volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
  vklSetVec3f(
      volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

  std::vector<unsigned int> timeConfiguration(9, 0);
  std::vector<float> timeSamples(20, 0.f);
  std::vector<float> voxels(20, 0.f);

  std::vector<VKLData> attributesData;
  std::vector<VKLData> attributesTimeData;
  std::vector<VKLData> attributesTimeConfig;

  size_t indexSum = 0;
  for (size_t z = 0; z < 2; z++) {
    for (size_t y = 0; y < 2; y++) {
      for (size_t x = 0; x < 2; x++) {
        size_t numTimeSamples                      = x == 0 ? 2 : 3;
        size_t index                               = z * 4 + y * 2 + x;
        timeConfiguration[index]                   = indexSum;
        timeSamples[indexSum]                      = 0.f;
        voxels[indexSum]                           = 0.f;
        timeSamples[indexSum + numTimeSamples - 1] = 1.0f;
        voxels[indexSum + numTimeSamples - 1]      = x + y + z;
        if (x == 1) {
          timeSamples[indexSum + 1] = 0.5f;
          voxels[indexSum + 1]      = 0.5f * (x + y + z);
        }
        indexSum += numTimeSamples;
      }
    }
  }
  timeConfiguration[8] = indexSum;

  VKLData attributeData = vklNewData(voxels.size(), VKL_FLOAT, voxels.data());
  attributesData.push_back(attributeData);
  VKLData attributeTimeConfig =
      vklNewData(timeConfiguration.size(), VKL_UINT, timeConfiguration.data());
  attributesTimeConfig.push_back(attributeTimeConfig);
  VKLData attributeTimeData =
      vklNewData(timeSamples.size(), VKL_FLOAT, timeSamples.data());
  attributesTimeData.push_back(attributeTimeData);

  std::vector<float>().swap(voxels);
  std::vector<float>().swap(timeSamples);
  std::vector<unsigned int>().swap(timeConfiguration);

  VKLData data =
      vklNewData(attributesData.size(), VKL_DATA, attributesData.data());
  VKLData timeData = vklNewData(
      attributesTimeData.size(), VKL_DATA, attributesTimeData.data());
  VKLData timeConfig = vklNewData(
      attributesTimeConfig.size(), VKL_DATA, attributesTimeConfig.data());

  for (const auto &d : attributesData) {
    vklRelease(d);
  }
  for (const auto &d : attributesTimeData) {
    if (d) {
      vklRelease(d);
    }
  }
  for (const auto &d : attributesTimeConfig) {
    vklRelease(d);
  }

  vklSetData(volume, "data", data);
  vklSetData(volume, "timeData", timeData);
  vklSetData(volume, "timeConfig", timeConfig);

  vklRelease(data);
  vklRelease(timeData);
  vklRelease(timeConfig);

  vklCommit(volume);

  const float sampleTolerance = 1e-4f;

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

inline void sampling_on_vertices_vs_procedural_values_motion_blur(
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

    for (float time : {0.f, .25f, .5f, .75f, 1.0f}) {
      INFO("time = " << time);
      proceduralValues.clear();
      for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
        if (a == 0) {
          proceduralValues.push_back(
              v->computeProceduralValue(objectCoordinates, a, 0.f));
        } else {
          proceduralValues.push_back(
              v->computeProceduralValue(objectCoordinates, a, time));
        }
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

TEST_CASE("Structured regular volume sampling with motion blur",
          "[volume_sampling]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  const vec3i dimensions(32);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (32.f - 1.f));

  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};

  for (const auto &dcf : dataCreationFlags) {
    std::stringstream sectionName;
    sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                            : "VKL_DATA_SHARED_BUFFER");

    DYNAMIC_SECTION(sectionName.str())
    {
      std::shared_ptr<TestingStructuredVolumeMulti> v(
          generateMultiAttributeStructuredRegularVolumeMB(
              dimensions, gridOrigin, gridSpacing, dcf));

      VKLVolume vklVolume = v->getVKLVolume();
      REQUIRE(vklGetNumAttributes(vklVolume) == v->getNumAttributes());
      sampling_on_vertices_vs_procedural_values_motion_blur(v, 2);
      // TODO, reenable: sampling_on_vertices_vs_procedural_values_varying_TUV_data();

      for (float time : {0.f, .25f, .5f, .75f, 1.0f}) {
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

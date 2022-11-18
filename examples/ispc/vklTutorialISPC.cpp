// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>
#include <openvkl/device/openvkl.h>
#include <iostream>
#include <vector>

#include "vklTutorialISPC_ispc.h"

#if defined(_MSC_VER)
#include <windows.h>  // Sleep
#endif

int main()
{
  vklInit();

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);

  vkl_vec3i dimensions{128, 128, 128};

  VKLVolume volume = vklNewVolume(device, "structuredRegular");
  vklSetVec3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  std::vector<float> voxels(dimensions.x * dimensions.y * dimensions.z);

  std::vector<VKLData> attributes;

  // volume attribute 0: x-grad
  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            (float)i;

  attributes.push_back(
      vklNewData(device, voxels.size(), VKL_FLOAT, voxels.data()));

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            (float)j;

  attributes.push_back(
      vklNewData(device, voxels.size(), VKL_FLOAT, voxels.data()));

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            (float)k;

  attributes.push_back(
      vklNewData(device, voxels.size(), VKL_FLOAT, voxels.data()));

  VKLData attributesData =
      vklNewData(device, attributes.size(), VKL_DATA, attributes.data());

  for (auto &attribute : attributes)
    vklRelease(attribute);

  vklSetData(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit(volume);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit2(sampler);

  // interval iterator context setup
  vkl_range1f ranges[2] = {{10, 20}, {50, 75}};
  int num_ranges        = 2;
  VKLData rangesData =
      vklNewData(device, num_ranges, VKL_BOX1F, ranges, VKL_DATA_DEFAULT, 0);

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetData(intervalContext, "valueRanges", rangesData);
  vklRelease(rangesData);

  vklCommit(intervalContext);

  // hit iterator context setup
  float values[2] = {32, 96};
  int num_values  = 2;
  VKLData valuesData =
      vklNewData(device, num_values, VKL_FLOAT, values, VKL_DATA_DEFAULT, 0);

  VKLHitIteratorContext hitContext = vklNewHitIteratorContext(sampler);

  vklSetData(hitContext, "values", valuesData);
  vklRelease(valuesData);

  vklCommit(hitContext);

  ispc::demo_ispc(
      volume, &sampler, intervalContext, hitContext, ranges, values);

  vklRelease(hitContext);
  vklRelease(intervalContext);
  vklRelease2(sampler);
  vklRelease(volume);

  vklReleaseDevice(device);

  printf("complete.\n");

#if defined(_MSC_VER)
  // On Windows, sleep for a few seconds so the terminal window doesn't close
  // immediately.
  Sleep(3000);
#endif

  return 0;
}

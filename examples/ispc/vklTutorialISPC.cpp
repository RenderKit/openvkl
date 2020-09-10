// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>
#include <iostream>
#include <vector>

#include "vklTutorialISPC_ispc.h"

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  vkl_vec3i dimensions{128, 128, 128};

  VKLVolume volume = vklNewVolume("structuredRegular");
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

  attributes.push_back(vklNewData(voxels.size(), VKL_FLOAT, voxels.data()));

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            (float)j;

  attributes.push_back(vklNewData(voxels.size(), VKL_FLOAT, voxels.data()));

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            (float)k;

  attributes.push_back(vklNewData(voxels.size(), VKL_FLOAT, voxels.data()));

  VKLData attributesData =
      vklNewData(attributes.size(), VKL_DATA, attributes.data());

  for (auto &attribute : attributes)
    vklRelease(attribute);

  vklSetData(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit(volume);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  ispc::demo_ispc(volume, sampler);

  vklRelease(sampler);
  vklRelease(volume);

  vklShutdown();

  return 0;
}

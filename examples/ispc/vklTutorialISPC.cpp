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

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  std::vector<float> voxels(dimensions.x * dimensions.y * dimensions.z);

  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            float(i);

  VKLData data = vklNewData(voxels.size(), VKL_FLOAT, voxels.data());
  vklSetData(volume, "data", data);
  vklRelease(data);

  vklCommit(volume);

  ispc::demo_ispc(volume, sampler);

  vklRelease(sampler);
  vklRelease(volume);

  vklShutdown();

  return 0;
}

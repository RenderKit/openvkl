// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <openvkl/openvkl.h>
#include <iostream>
#include <vector>

#include "vklTutorialISPC_ispc.h"

int main()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  vkl_vec3i dimensions{128, 128, 128};

  VKLVolume volume = vklNewVolume("structured_regular");
  vklSet3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vklSet3f(volume, "gridOrigin", 0, 0, 0);
  vklSet3f(volume, "gridSpacing", 1, 1, 1);

  std::vector<float> voxels(dimensions.x * dimensions.y * dimensions.z);

  for (int k = 0; k < dimensions.z; k++)
    for (int j = 0; j < dimensions.y; j++)
      for (int i = 0; i < dimensions.x; i++)
        voxels[k * dimensions.x * dimensions.y + j * dimensions.x + i] =
            float(i);

  VKLData voxelData = vklNewData(voxels.size(), VKL_FLOAT, voxels.data());
  vklSetData(volume, "voxelData", voxelData);
  vklRelease(voxelData);

  vklCommit(volume);

  ispc::sample_ispc((ispc::OpenVKLVolume *)volume);

  vklShutdown();

  return 0;
}

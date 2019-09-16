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

  VKLVolume volume = vklNewVolume("amr_volume");
  vklSetVec3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
  vklSetVec3f(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f(volume, "gridSpacing", 1, 1, 1);

  std::vector<float> voxels(dimensions.x * dimensions.y * dimensions.z);
  std::vector<std::vector<float>> blockDataVectors(8);
  std::vector<VKLData> blockData;

  for (int b = 0; b < 8; b++)
    for (int k = 0; k < dimensions.z / 2; k++)
      for (int j = 0; j < dimensions.y / 2; j++)
        for (int i = 0; i < dimensions.x / 2; i++)
          blockDataVectors[b].push_back(float(i));

  for (auto &bd : blockDataVectors)
    blockData.push_back(vklNewData(bd.size(), VKL_FLOAT, bd.data()));

  // 8 64^3 boxes
  std::vector<int> bounds{  0,   0,   0,  63,  63,  63,
                           64,   0,   0, 127,  63,  63,
                            0,  64,   0,  63, 127,  63,
                           64,  64,   0, 127, 127,  63,
                            0,   0,  64,  63,  63, 127,
                           64,   0,  64, 127,  63, 127,
                            0,  64,  64,  63, 127, 127,
                           64,  64,  64, 127, 127, 127};
  std::vector<int> levels{0, 0, 0, 0, 0, 0, 0, 0};
  std::vector<float> widths{1.f};

  VKLData boundsData = vklNewData(bounds.size(), VKL_INT, bounds.data());
  vklSetData(volume, "block.bounds", boundsData);
  vklRelease(boundsData);

  VKLData levelsData = vklNewData(levels.size(), VKL_INT, levels.data());
  vklSetData(volume, "block.level", levelsData);
  vklRelease(levelsData);

  VKLData cellWidthsData = vklNewData(widths.size(), VKL_FLOAT, widths.data());
  vklSetData(volume, "block.cellWidth", cellWidthsData);
  vklRelease(cellWidthsData);

  VKLData blockDataData =
      vklNewData(blockData.size(), VKL_DATA, blockData.data());
  vklSetData(volume, "block.data", blockDataData);
  for (auto &bd : blockData)
    vklRelease(bd);
  vklRelease(blockDataData);

  vklSetInt(volume, "voxelType", VKL_FLOAT);

  vklCommit(volume);

  ispc::sample_ispc((ispc::OpenVKLVolume *)volume);

  vklShutdown();

  return 0;
}

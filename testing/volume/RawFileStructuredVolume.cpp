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

#include "RawFileStructuredVolume.h"

namespace volley {
  namespace testing {

    RawFileStructuredVolume::RawFileStructuredVolume(
        const std::string &filename,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing)
        : filename(filename),
          TestingStructuredVolume(dimensions, gridOrigin, gridSpacing)
    {
      std::vector<float> voxels = generateVoxels();

      volume = vlyNewVolume("shared_structured_volume");

      vlySet3i(volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vlySet3f(volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vlySet3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      VLYData voxelData = vlyNewData(voxels.size(), VLY_FLOAT, voxels.data());
      vlySetData(volume, "voxelData", voxelData);
      vlyRelease(voxelData);

      vlyCommit(volume);
    }

    std::vector<float> RawFileStructuredVolume::generateVoxels()
    {
      std::vector<float> voxels(dimensions.product());

      std::ifstream input(filename, std::ios::binary);

      if (!input) {
        throw std::runtime_error("error opening raw volume file");
      }

      input.read((char *)voxels.data(), dimensions.product() * sizeof(float));

      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }

      return voxels;
    }

  }  // namespace testing
}  // namespace volley

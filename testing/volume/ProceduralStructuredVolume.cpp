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

#include "ProceduralStructuredVolume.h"

namespace volley {
  namespace testing {

    template <float volumeSamplingFunction(const vec3f &)>
    ProceduralStructuredVolume<volumeSamplingFunction>::
        ProceduralStructuredVolume(const vec3i &dimensions,
                                   const vec3f &gridOrigin,
                                   const vec3f &gridSpacing)
        : dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing)
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

    template <float volumeSamplingFunction(const vec3f &)>
    std::vector<float>
    ProceduralStructuredVolume<volumeSamplingFunction>::generateVoxels()
    {
      std::vector<float> voxels(dimensions.product());

      auto transformLocalToObject = [&](const vec3f &localCoordinates) {
        return gridOrigin + localCoordinates * gridSpacing;
      };

      for (size_t z = 0; z < dimensions.z; z++) {
        for (size_t y = 0; y < dimensions.y; y++) {
          for (size_t x = 0; x < dimensions.x; x++) {
            size_t index =
                z * dimensions.y * dimensions.x + y * dimensions.x + x;
            vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
            voxels[index]           = volumeSamplingFunction(objectCoordinates);
          }
        }
      }

      return voxels;
    }

    template class ProceduralStructuredVolume<getWaveletValue>;
  }  // namespace testing
}  // namespace volley

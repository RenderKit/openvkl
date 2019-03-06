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

#include "StructuredVolume.h"
#include <cmath>

namespace volley {
  namespace ispc_driver {

    void StructuredVolume::commit()
    {
      Volume::commit();

      samplingMethod =
          VLYSamplingMethod(getParam<int>("samplingMethod", VLY_SAMPLE_LINEAR));

      gridOrigin  = getParam<vec3f>("gridOrigin", vec3f(0.f));
      gridSpacing = getParam<vec3f>("gridSpacing", vec3f(1.f));
    }

    float StructuredVolume::computeSample(const vec3f &objectCoordinates) const
    {
      // Transform the sample location into the local coordinate system.
      vec3f localCoordinates = transformObjectToLocal(objectCoordinates);

      // Coordinates outside the volume are clamped to the volume bounds.
      const vec3f clampedLocalCoordinates =
          clamp(localCoordinates, vec3f(0.0f), localCoordinatesUpperBound);

      // Lower and upper corners of the box straddling the voxels to be
      // interpolated.
      const vec3i voxelIndex_0 = vec3i(clampedLocalCoordinates);
      const vec3i voxelIndex_1 = voxelIndex_0 + 1;

      // Fractional coordinates within the lower corner voxel used during
      // interpolation.
      const vec3f fractionalLocalCoordinates =
          clampedLocalCoordinates - vec3f(voxelIndex_0);

      // Look up the voxel values to be interpolated.
      const float voxelValue_000 =
          getVoxel(vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z));
      const float voxelValue_001 =
          getVoxel(vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z));
      const float voxelValue_010 =
          getVoxel(vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z));
      const float voxelValue_011 =
          getVoxel(vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z));
      const float voxelValue_100 =
          getVoxel(vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z));
      const float voxelValue_101 =
          getVoxel(vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z));
      const float voxelValue_110 =
          getVoxel(vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z));
      const float voxelValue_111 =
          getVoxel(vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z));

      // Interpolate the voxel values.
      const float voxelValue_00 =
          voxelValue_000 +
          fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
      const float voxelValue_01 =
          voxelValue_010 +
          fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
      const float voxelValue_10 =
          voxelValue_100 +
          fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
      const float voxelValue_11 =
          voxelValue_110 +
          fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
      const float voxelValue_0 =
          voxelValue_00 +
          fractionalLocalCoordinates.y * (voxelValue_01 - voxelValue_00);
      const float voxelValue_1 =
          voxelValue_10 +
          fractionalLocalCoordinates.y * (voxelValue_11 - voxelValue_10);

      const float sample = voxelValue_0 + fractionalLocalCoordinates.z *
                                              (voxelValue_1 - voxelValue_0);

      return sample;
    }

    vec3f StructuredVolume::computeGradient(
        const vec3f &objectCoordinates) const
    {
      // TODO
      return vec3f(0.f, 0.f, 0.f);
    }

  }  // namespace ispc_driver
}  // namespace volley

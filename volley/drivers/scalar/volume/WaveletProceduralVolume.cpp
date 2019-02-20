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

#include "WaveletProceduralVolume.h"
#include <cmath>

namespace volley {
  namespace scalar_driver {

    void WaveletProceduralVolume::commit()
    {
      StructuredVolume::commit();

      // this volume is generated on commit based on the user-provided
      // dimensions
      dimensions = getParam<vec3i>("dimensions", vec3i(128, 128, 128));
      localCoordinatesUpperBound = nextafter(dimensions, vec3i(0, 0, 0));

      volumeData.resize(dimensions.x * dimensions.y * dimensions.z);

      for (size_t z = 0; z < dimensions.z; z++) {
        for (size_t y = 0; y < dimensions.y; y++) {
          for (size_t x = 0; x < dimensions.x; x++) {
            size_t index =
                z * dimensions.y * dimensions.x + y * dimensions.x + x;
            vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
            volumeData[index]       = getWaveletValue(objectCoordinates);
          }
        }
      }
    }

    float WaveletProceduralVolume::getVoxel(const vec3i &localCoordinates) const
    {
      size_t index = localCoordinates.z * dimensions.y * dimensions.x +
                     localCoordinates.y * dimensions.x + localCoordinates.x;
      return volumeData[index];
    }

    float WaveletProceduralVolume::getWaveletValue(
        const vec3f &objectCoordinates) const
    {
      return M * G *
             (XM * sinf(XF * objectCoordinates.x) +
              YM * sinf(YF * objectCoordinates.y) +
              ZM * cosf(ZF * objectCoordinates.z));
    }

    VLY_REGISTER_VOLUME(WaveletProceduralVolume, wavelet_procedural_volume)

  }  // namespace scalar_driver
}  // namespace volley

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
  namespace ispc_driver {

    void WaveletProceduralVolume::commit()
    {
      // these parameters are typically set in a superclass, but we need these
      // before we call the superclass commit()
      dimensions  = getParam<vec3i>("dimensions", vec3i(128));
      gridOrigin  = getParam<vec3f>("gridOrigin", vec3f(0.f));
      gridSpacing = getParam<vec3f>("gridSpacing", vec3f(1.f));

      generatedData.resize(dimensions.x * dimensions.y * dimensions.z);

      auto transformLocalToObject = [&](const vec3f &localCoordinates) {
        return gridOrigin + localCoordinates * gridSpacing;
      };

      for (size_t z = 0; z < dimensions.z; z++) {
        for (size_t y = 0; y < dimensions.y; y++) {
          for (size_t x = 0; x < dimensions.x; x++) {
            size_t index =
                z * dimensions.y * dimensions.x + y * dimensions.x + x;
            vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
            generatedData[index]    = getWaveletValue(objectCoordinates);
          }
        }
      }

      // helper code used to write procedurally-generated data files to disk for
      // performance comparisons in other volume implementations
      std::string saveFilename = getParam<std::string>("saveFilename", "");

      if (!saveFilename.empty()) {
        std::ofstream out;
        out.open(saveFilename, std::ios::binary);
        out.write(reinterpret_cast<char *>(generatedData.data()),
                  generatedData.size() * sizeof(float));
        out.close();
      }

      // populate data object for SharedStructuredVolume
      voxelData = new Data(generatedData.size(),
                           VLY_FLOAT,
                           generatedData.data(),
                           VLY_DATA_SHARED_BUFFER);

      SharedStructuredVolume::commit();
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

  }  // namespace ispc_driver
}  // namespace volley

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

#pragma once

#include <vector>
#include "ospray/ospcommon/vec.h"
#include "volley/volley.h"

using namespace ospcommon;

namespace volley {
  namespace testing {

    template <float volumeSamplingFunction(const vec3f &)>
    struct ProceduralStructuredVolume
    {
      ProceduralStructuredVolume(const vec3i &dimensions,
                                 const vec3f &gridOrigin,
                                 const vec3f &gridSpacing);

      inline vec3i getDimensions() const
      {
        return dimensions;
      }

      inline vec3f getGridOrigin() const
      {
        return gridOrigin;
      }

      inline vec3f getGridSpacing() const
      {
        return gridSpacing;
      }

      inline VLYVolume getVLYVolume() const
      {
        return volume;
      }

      inline float computeProceduralValue(const vec3f &objectCoordinates)
      {
        return volumeSamplingFunction(objectCoordinates);
      }

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      std::vector<float> generateVoxels();

     protected:
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VLYVolume volume{nullptr};
    };

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline float getWaveletValue(const vec3f &objectCoordinates)
    {
      // wavelet parameters
      constexpr float M  = 1.f;
      constexpr float G  = 1.f;
      constexpr float XM = 1.f;
      constexpr float YM = 1.f;
      constexpr float ZM = 1.f;
      constexpr float XF = 3.f;
      constexpr float YF = 3.f;
      constexpr float ZF = 3.f;

      return M * G *
             (XM * sinf(XF * objectCoordinates.x) +
              YM * sinf(YF * objectCoordinates.y) +
              ZM * cosf(ZF * objectCoordinates.z));
    }

    inline float getZValue(const vec3f &objectCoordinates)
    {
      return objectCoordinates.z;
    }

    using WaveletProceduralVolume = ProceduralStructuredVolume<getWaveletValue>;
    using ZProceduralVolume = ProceduralStructuredVolume<getZValue>;

  }  // namespace testing
}  // namespace volley

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

#include "TestingAMRVolume.h"
#include "procedural_functions.h"
// openvkl
#include "openvkl/openvkl.h"
// ospcommon
#include "ospcommon/math/vec.h"
#include "ospcommon/tasking/parallel_for.h"
// std
#include <algorithm>
#include <vector>

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &, const vec3i &),
              vec3f volumeGradientFunction(const vec3f &) =
                  gradientNotImplemented>
    struct ProceduralAMRVolume : public TestingAMRVolume
    {
      ProceduralAMRVolume(const vec3i &_dimensions,
                          const vec3f &_gridOrigin,
                          const vec3f &_gridSpacing);

      VOXEL_TYPE computeProceduralValue(const vec3f &objectCoordinates);

      vec3f computeProceduralGradient(const vec3f &objectCoordinates);

      std::vector<unsigned char> generateVoxels() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &,
                                                const vec3i &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline ProceduralAMRVolume<
        VOXEL_TYPE,
        volumeSamplingFunction,
        volumeGradientFunction>::ProceduralAMRVolume(const vec3i &_dimensions,
                                                     const vec3f &_gridOrigin,
                                                     const vec3f &_gridSpacing)
        : TestingAMRVolume(_dimensions,
                           _gridOrigin,
                           _gridSpacing,
                           getVKLDataType<VOXEL_TYPE>())
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, const vec3i &),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return samplingFunction(objectCoordinates, dimensions);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, const vec3i &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates)
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &, const vec3i &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralAMRVolume<VOXEL_TYPE,
                        volumeSamplingFunction,
                        volumeGradientFunction>::generateVoxels()
    {
      {
        auto numValues = longProduct(this->dimensions);
        std::vector<unsigned char> voxels(numValues * sizeof(VOXEL_TYPE));

        VOXEL_TYPE *voxelsTyped = (VOXEL_TYPE *)voxels.data();

        auto transformLocalToObject = [&](const vec3f &localCoordinates) {
          return this->gridOrigin + localCoordinates * this->gridSpacing;
        };

        ospcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
          for (size_t y = 0; y < this->dimensions.y; y++) {
            for (size_t x = 0; x < this->dimensions.x; x++) {
              size_t index = z * this->dimensions.y * this->dimensions.x +
                             y * this->dimensions.x + x;
              vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
              voxelsTyped[index] = volumeSamplingFunction(objectCoordinates, dimensions);
            }
          }
        });

        auto minmax = std::minmax_element(voxelsTyped, voxelsTyped + numValues);

        return voxels;
      }
    }
  }  // namespace testing
}  // namespace openvkl

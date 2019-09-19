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

#include "../utility/rawToAMR.h"
#include "TestingVolume.h"
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
              VOXEL_TYPE volumeSamplingFunction(const vec3f &),
              vec3f volumeGradientFunction(const vec3f &) =
                  gradientNotImplemented>
    struct ProceduralAMRVolume : public TestingVolume
    {
      ProceduralAMRVolume(const vec3i &_dimensions,
                          const vec3f &_gridOrigin,
                          const vec3f &_gridSpacing);

      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VOXEL_TYPE computeProceduralValue(const vec3f &objectCoordinates);

      vec3f computeProceduralGradient(const vec3f &objectCoordinates);

      std::vector<unsigned char> generateVoxels();

     protected:
      void generateVKLVolume() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline ProceduralAMRVolume<
        VOXEL_TYPE,
        volumeSamplingFunction,
        volumeGradientFunction>::ProceduralAMRVolume(const vec3i &_dimensions,
                                                     const vec3f &_gridOrigin,
                                                     const vec3f &_gridSpacing)
        : dimensions(_dimensions),
          gridOrigin(_gridOrigin),
          gridSpacing(_gridSpacing)
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline VOXEL_TYPE
    ProceduralAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return samplingFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates)
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &),
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
              voxelsTyped[index] = volumeSamplingFunction(objectCoordinates);
            }
          }
        });

        auto minmax = std::minmax_element(voxelsTyped, voxelsTyped + numValues);

        return voxels;
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline void ProceduralAMRVolume<VOXEL_TYPE,
                                    volumeSamplingFunction,
                                    volumeGradientFunction>::generateVKLVolume()
    {
      std::vector<unsigned char> voxels = generateVoxels();

      // create AMR representation of procedurally generated voxels

      // input parameters for AMR conversion
      const int numLevels = 1;   // number of refinement levels to create
      const int blockSize = 16;  // edge extent of a block (cube)
      const int refFactor = 4;   // refinement factor, i.e. scale between levels
      const float threshold = 1.0f;  // value range threshold to refine at

      // output containers from AMR conversion
      std::vector<box3i> blockBounds;  // object space bounds for AMR blocks
      std::vector<int> refLevels;      // refinement hierarchy levels for blocks
      std::vector<float> cellWidths;   // width of cell at refinement level i
      std::vector<std::vector<float>> blockValues;  // data values per block
      std::vector<VKLData> blockData;  // data values per block as VKLData

      float *floatData = (float *)voxels.data();
      std::vector<float> floatVoxels;
      floatVoxels.assign(floatData, floatData + longProduct(dimensions));

      makeAMR(floatVoxels,
              dimensions,
              numLevels,
              blockSize,
              refFactor,
              threshold,
              blockBounds,
              refLevels,
              cellWidths,
              blockValues);

      // convert vector<float> to VKLData
      for (const auto &bv : blockValues)
        blockData.push_back(vklNewData(bv.size(), VKL_FLOAT, bv.data()));

      // create a nested VKLData array. This is what gets passed to AMRVolume
      VKLData blockDataData =
          vklNewData(blockData.size(), VKL_DATA, blockData.data());

      // create the other VKLData arrays
      VKLData boundsData =
          vklNewData(blockBounds.size(), VKL_BOX3I, blockBounds.data());
      VKLData levelsData =
          vklNewData(refLevels.size(), VKL_INT, refLevels.data());
      VKLData widthsData =
          vklNewData(cellWidths.size(), VKL_FLOAT, cellWidths.data());

      // create the VKL AMR volume
      volume = vklNewVolume("amr_volume");

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);
      vklSetInt(volume, "voxelType", VKL_FLOAT);
      vklSetData(volume, "block.data", blockDataData);
      vklSetData(volume, "block.bounds", boundsData);
      vklSetData(volume, "block.level", levelsData);
      vklSetData(volume, "block.cellWidth", widthsData);

      vklCommit(volume);
    }
  }  // namespace testing
}  // namespace openvkl

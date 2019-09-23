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
    struct ProceduralShellsAMRVolume : public TestingAMRVolume
    {
      ProceduralShellsAMRVolume(const vec3i &_dimensions,
                          const vec3f &_gridOrigin,
                          const vec3f &_gridSpacing);

      VOXEL_TYPE computeProceduralValue(const vec3f &objectCoordinates);

      vec3f computeProceduralGradient(const vec3f &objectCoordinates);

      std::vector<unsigned char> generateVoxels() override;  // unused

     protected:
      void generateVKLVolume();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &, const vec3i &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline ProceduralShellsAMRVolume<
        VOXEL_TYPE,
        volumeSamplingFunction,
        volumeGradientFunction>::ProceduralShellsAMRVolume(const vec3i &_dimensions,
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
    ProceduralShellsAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return samplingFunction(objectCoordinates, dimensions);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, const vec3i &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralShellsAMRVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates)
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &, const vec3i &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralShellsAMRVolume<VOXEL_TYPE,
                        volumeSamplingFunction,
                        volumeGradientFunction>::generateVoxels()
    {
      {
        return std::vector<unsigned char>();
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE volumeSamplingFunction(const vec3f &, const vec3i &),
              vec3f volumeGradientFunction(const vec3f &)>
    inline void ProceduralShellsAMRVolume<VOXEL_TYPE,
                                    volumeSamplingFunction,
                                    volumeGradientFunction>::generateVKLVolume()
    {
      std::vector<box3i> blockBounds;
      std::vector<int> refinementLevels;
      std::vector<float> cellWidths;
      std::vector<std::vector<float>> blockDataVectors;
      std::vector<VKLData> blockData;

      // block bound upper bounds are inclusive

      // outer shell - takes entire world space region
      // lowest refinement level, contains 16^3 cells with cell width 16 to
      // create a 256^3 region
      blockBounds.emplace_back(vec3i(0), vec3i(15));
      refinementLevels.emplace_back(0);
      cellWidths.emplace_back(16.f);
      std::vector<float> voxels(4096, 1.f);
      blockDataVectors.emplace_back(voxels);

      // middle shell - 8 16^3 blocks with cell width 4 at the center of the
      // region. note that the bounds are scaled by the cell width, so these
      // blocks take up the area from (64,64,64) to (191, 191, 191)
      blockBounds.emplace_back(vec3i(16), vec3i(31));
      refinementLevels.emplace_back(1);
      cellWidths.emplace_back(4.f);
      voxels = std::vector<float>(4096, 5.f);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(16, 16, 32), vec3i(31, 31, 47));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(16, 32, 16), vec3i(31, 47, 31));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(16, 32, 32), vec3i(31, 47, 47));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(32, 16, 16), vec3i(47, 31, 31));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(32, 16, 32), vec3i(47, 31, 47));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(32, 32, 16), vec3i(47, 47, 31));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(32), vec3i(47));
      refinementLevels.emplace_back(1);
      blockDataVectors.emplace_back(voxels);

      // core - inner 8 16^3 blocks with cell width 1 at the finest refinement
      // level, takes world space region (112,112,112) to (144,144,144)
      blockBounds.emplace_back(vec3i(112), vec3i(127));
      refinementLevels.emplace_back(2);
      cellWidths.emplace_back(1.f);
      voxels = std::vector<float>(4096, 10.f);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(112, 112, 128), vec3i(127, 127, 143));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(112, 128, 112), vec3i(127, 143, 127));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(112, 128, 128), vec3i(127, 143, 143));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(128, 112, 112), vec3i(143, 127, 127));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(128, 112, 128), vec3i(143, 127, 143));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(128, 128, 112), vec3i(143, 143, 127));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      blockBounds.emplace_back(vec3i(128), vec3i(143));
      refinementLevels.emplace_back(2);
      blockDataVectors.emplace_back(voxels);

      // convert the data above to VKLData objects

      for (const std::vector<float> &bd : blockDataVectors) {
        VKLData data = vklNewData(bd.size(), VKL_FLOAT, bd.data());
        blockData.push_back(data);
      }

      VKLData blockDataData =
          vklNewData(blockData.size(), VKL_DATA, blockData.data());

      VKLData blockBoundsData =
          vklNewData(blockBounds.size(), VKL_BOX3I, blockBounds.data());

      VKLData refinementLevelsData =
          vklNewData(refinementLevels.size(), VKL_INT, refinementLevels.data());

      VKLData cellWidthsData =
          vklNewData(cellWidths.size(), VKL_FLOAT, cellWidths.data());

      // create the AMR volume

      volume = vklNewVolume("amr_volume");

      vklSetInt(volume, "voxelType", VKL_FLOAT);
      vklSetData(volume, "block.data", blockDataData);
      vklSetData(volume, "block.bounds", blockBoundsData);
      vklSetData(volume, "block.level", refinementLevelsData);
      vklSetData(volume, "block.cellWidth", cellWidthsData);

      vklCommit(volume);
    }
  }  // namespace testing
}  // namespace openvkl

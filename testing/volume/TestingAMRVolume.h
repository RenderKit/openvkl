// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include <mutex>
#include <vector>
// openvkl
#include "TestingVolume.h"
#include "openvkl/common.h"
// ospcommon
#include "ospcommon/math/box.h"
#include "ospcommon/math/range.h"
#include "ospcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace testing {

    struct TestingAMRVolume : public TestingVolume
    {
      TestingAMRVolume(const vec3i &dimensions,
                       const vec3f &gridOrigin,
                       const vec3f &gridSpacing);

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<unsigned char> generateVoxels() = 0;

     protected:
      void generateVKLVolume() override;

      // convert structured volume data to AMR representation
      void makeAMR(const std::vector<float> &voxels,
                   const vec3i inGridDims,
                   const int numLevels,
                   const int blockSize,
                   const int refinementLevel,
                   const float threshold,
                   std::vector<box3i> &blockBounds,
                   std::vector<int> &refinementLevels,
                   std::vector<float> &cellWidths,
                   std::vector<std::vector<float>> &brickData);

      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingAMRVolume::TestingAMRVolume(const vec3i &dimensions,
                                              const vec3f &gridOrigin,
                                              const vec3f &gridSpacing)

        : dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing)
    {
    }

    inline vec3i TestingAMRVolume::getDimensions() const
    {
      return dimensions;
    }

    inline vec3f TestingAMRVolume::getGridOrigin() const
    {
      return gridOrigin;
    }

    inline vec3f TestingAMRVolume::getGridSpacing() const
    {
      return gridSpacing;
    }

    inline void TestingAMRVolume::generateVKLVolume()
    {
      std::vector<unsigned char> voxels = generateVoxels();

      // create AMR representation of procedurally generated voxels

      // input parameters for AMR conversion
      const int numLevels = 3;   // number of refinement levels to create
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

      volume = vklNewVolume("amr");

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);
      vklSetData(volume, "block.data", blockDataData);
      vklSetData(volume, "block.bounds", boundsData);
      vklSetData(volume, "block.level", levelsData);
      vklSetData(volume, "block.cellWidth", widthsData);

      vklCommit(volume);
    }

    inline void TestingAMRVolume::makeAMR(
        const std::vector<float> &voxels,
        const vec3i inGridDims,
        const int numLevels,
        const int blockSize,
        const int refinementLevel,
        const float threshold,
        std::vector<box3i> &blockBounds,
        std::vector<int> &refinementLevels,
        std::vector<float> &cellWidths,
        std::vector<std::vector<float>> &brickData)
    {
      int minWidth = blockSize;

      for (int i = 1; i < numLevels; i++)
        minWidth *= refinementLevel;

      if (minWidth >= refinementLevel * reduce_max(inGridDims)) {
        throw std::runtime_error(
            "too many levels, or too fine a refinement factor."
            "do not have a single brick at the root...");
      }

      vec3i finestLevelSize = vec3i(minWidth);
      while (finestLevelSize.x < inGridDims.x)
        finestLevelSize.x += minWidth;
      while (finestLevelSize.y < inGridDims.y)
        finestLevelSize.y += minWidth;
      while (finestLevelSize.z < inGridDims.z)
        finestLevelSize.z += minWidth;

      std::cout << "grid dimensions: " << inGridDims.x << " " << inGridDims.y
                << " " << inGridDims.z << std::endl;
      std::cout << "min width: " << minWidth << std::endl;
      std::cout << "finest level size: " << finestLevelSize.x << " "
                << finestLevelSize.y << " " << finestLevelSize.z << std::endl;

      // create container for current level so we don't use voxels
      std::vector<float> &currentLevel =
          const_cast<std::vector<float> &>(voxels);

      std::mutex blockMutex;

      // create and write the bricks
      vec3i levelSize = finestLevelSize;
      for (int level = numLevels - 1; level >= 0; --level) {
        const vec3i nextLevelSize = levelSize / refinementLevel;
        // create container for next level down
        std::vector<float> nextLevel =
            std::vector<float>(nextLevelSize.product(), 0);

        const vec3i numBricks = levelSize / blockSize;
        ospcommon::tasking::parallel_for(
            numBricks.product(), [&](int brickIdx) {
              // dt == cellWidth in osp_amr_brick_info
              float dt = powf(refinementLevel, numLevels - level - 1);
              // get 3D brick index from flat brickIdx
              const vec3i brickID(brickIdx % numBricks.x,
                                  (brickIdx / numBricks.x) % numBricks.y,
                                  brickIdx / (numBricks.x * numBricks.y));
              // set upper and lower bounds of brick based on 3D index and
              // brick size in input data space
              box3i box;
              box.lower = brickID * blockSize;
              box.upper = box.lower + (blockSize - 1);
              // current brick data
              std::vector<float> data(blockSize * blockSize * blockSize);
              size_t out = 0;
              range1f brickRange;
              // traverse the data by brick index
              for (int iz = box.lower.z; iz <= box.upper.z; iz++) {
                for (int iy = box.lower.y; iy <= box.upper.y; iy++) {
                  for (int ix = box.lower.x; ix <= box.upper.x; ix++) {
                    const size_t thisLevelCoord =
                        ix + levelSize.y * (iy + iz * levelSize.z);
                    const size_t nextLevelCoord =
                        ix / refinementLevel +
                        nextLevelSize.y *
                            (iy / refinementLevel +
                             iz / refinementLevel * nextLevelSize.z);
                    // get the actual data at current coordinates
                    const float v = currentLevel[thisLevelCoord];
                    // insert the data into the current brick
                    data[out++] = v;
                    nextLevel[nextLevelCoord] +=
                        v /
                        (refinementLevel * refinementLevel * refinementLevel);
                    // extend the value range of this brick (min/max) as
                    // needed
                    brickRange.extend(v);
                  }
                }
              }

              std::lock_guard<std::mutex> lock(blockMutex);
              if (!((level > 0) &&
                    ((brickRange.upper - brickRange.lower) <= threshold))) {
                blockBounds.push_back(box);
                refinementLevels.push_back(level);
                cellWidths.resize(
                    std::max(cellWidths.size(), (size_t)level + 1));
                cellWidths[level] = dt;
                brickData.push_back(data);
              }
            });  // end parallel for
        currentLevel = nextLevel;
        levelSize    = nextLevelSize;
      }  // end for loop on levels
    }

  }  // namespace testing
}  // namespace openvkl

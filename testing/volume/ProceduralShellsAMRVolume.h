// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ProceduralVolume.h"
#include "TestingAMRVolume.h"
#include "procedural_functions.h"
// openvkl
#include "openvkl/openvkl.h"
// rkcommon
#include "rkcommon/math/vec.h"
#include "rkcommon/tasking/parallel_for.h"
// std
#include <algorithm>
#include <vector>

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    template <vec3f volumeGradientFunction(const vec3f &, float) =
                  gradientNotImplemented>
    struct ProceduralShellsAMRVolume : public TestingAMRVolume,
                                       public ProceduralVolume
    {
      ProceduralShellsAMRVolume(const vec3i &_dimensions,
                                const vec3f &_gridOrigin,
                                const vec3f &_gridSpacing);

      std::vector<unsigned char> generateVoxels() override;  // unused

     protected:
      void generateVKLVolume(VKLDevice device) override;

      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override;

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          float time) const override;

      int refFactor      = 4;
      int blockSize      = 16;
      const int numCells = blockSize * blockSize * blockSize;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <vec3f volumeGradientFunction(const vec3f &, float)>
    inline ProceduralShellsAMRVolume<volumeGradientFunction>::
        ProceduralShellsAMRVolume(const vec3i &_dimensions,
                                  const vec3f &_gridOrigin,
                                  const vec3f &_gridSpacing)
        : TestingAMRVolume(_dimensions, _gridOrigin, _gridSpacing),
          ProceduralVolume(false)
    {
      if (dimensions.x % blockSize != 0 || dimensions.y % blockSize != 0 ||
          dimensions.z % blockSize != 0) {
        std::stringstream ss;
        ss << "ProceduralShellsAMRVolume requires multiple-of-" << blockSize
           << " dimensions";
        throw std::runtime_error(ss.str());
      }
    }

    template <vec3f volumeGradientFunction(const vec3f &, float)>
    inline std::vector<unsigned char>
    ProceduralShellsAMRVolume<volumeGradientFunction>::generateVoxels()
    {
      {
        return std::vector<unsigned char>();
      }
    }

    template <vec3f volumeGradientFunction(const vec3f &, float)>
    inline void
    ProceduralShellsAMRVolume<volumeGradientFunction>::generateVKLVolume(
        VKLDevice device)
    {
      int numLevels = 0;
      int minExtent = reduce_min(dimensions);
      if (minExtent < 128) {
        numLevels = 2;
      } else if (minExtent < 256) {
        numLevels = 3;
        blockSize = 8;
      } else {
        numLevels = 3;
      }

      std::vector<box3i> blockBounds;
      std::vector<int> refinementLevels;
      std::vector<float> cellWidths;
      std::vector<std::vector<float>> blockDataVectors;
      std::vector<VKLData> blockData;

      // block bound upper bounds are inclusive, hence subtracting 1

      float cellWidth = gridSpacing.x * float(minExtent) / blockSize;
      int refLevel    = 0;
      std::vector<float> voxels(numCells, -0.5f);

      // outer shell - takes entire world space region
      blockBounds.emplace_back(vec3i(0), vec3i(blockSize - 1));
      refinementLevels.emplace_back(refLevel);
      cellWidths.emplace_back(cellWidth);
      blockDataVectors.emplace_back(voxels);

      // for each subsequent shell, create 8 16^3 blocks with progressively
      // smaller cell widths
      for (int level = 1; level < numLevels; level++) {
        cellWidth /= refFactor;
        cellWidths.emplace_back(cellWidth);

        voxels = std::vector<float>(numCells, float(level - 1));

        int lb = minExtent / 2 / std::pow(refFactor, numLevels - level - 1);
        int ub = lb + blockSize - 1;

        blockBounds.emplace_back(vec3i(lb), vec3i(ub));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb - blockSize, lb, lb),
                                 vec3i(ub - blockSize, ub, ub));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb, lb - blockSize, lb),
                                 vec3i(ub, ub - blockSize, ub));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb - blockSize, lb - blockSize, lb),
                                 vec3i(ub - blockSize, ub - blockSize, ub));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb, lb, lb - blockSize),
                                 vec3i(ub, ub, ub - blockSize));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb - blockSize, lb, lb - blockSize),
                                 vec3i(ub - blockSize, ub, ub - blockSize));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb, lb - blockSize, lb - blockSize),
                                 vec3i(ub, ub - blockSize, ub - blockSize));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);

        blockBounds.emplace_back(vec3i(lb - blockSize), vec3i(ub - blockSize));
        refinementLevels.emplace_back(level);
        blockDataVectors.emplace_back(voxels);
      }

      // convert the data above to VKLData objects

      for (const std::vector<float> &bd : blockDataVectors)
        blockData.push_back(
            vklNewData(device, bd.size(), VKL_FLOAT, bd.data()));

      VKLData blockDataData =
          vklNewData(device, blockData.size(), VKL_DATA, blockData.data());

      VKLData blockBoundsData =
          vklNewData(device, blockBounds.size(), VKL_BOX3I, blockBounds.data());

      VKLData refinementLevelsData = vklNewData(
          device, refinementLevels.size(), VKL_INT, refinementLevels.data());

      VKLData cellWidthsData =
          vklNewData(device, cellWidths.size(), VKL_FLOAT, cellWidths.data());

      // create the AMR volume

      volume = vklNewVolume(device, "amr");

      vklSetData(volume, "block.data", blockDataData);
      vklSetData(volume, "block.bounds", blockBoundsData);
      vklSetData(volume, "block.level", refinementLevelsData);
      vklSetData(volume, "cellWidth", cellWidthsData);

      vklRelease(blockDataData);
      vklRelease(blockBoundsData);
      vklRelease(refinementLevelsData);
      vklRelease(cellWidthsData);

      for (auto &d : blockData)
        vklRelease(d);

      vklCommit(volume);

      for (const auto &bdv : blockDataVectors)
        computedValueRange.extend(
            computeValueRange(VKL_FLOAT, bdv.data(), bdv.size()));
    }

    template <vec3f gradientFunction(const vec3f &, float)>
    inline float
    ProceduralShellsAMRVolume<gradientFunction>::computeProceduralValueImpl(
        const vec3f &objectCoordinates, float time) const
    {
      return getShellValue(objectCoordinates, dimensions);
    }

    template <vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralShellsAMRVolume<gradientFunction>::computeProceduralGradientImpl(
        const vec3f &objectCoordinates, float time) const
    {
      return gradientFunction(objectCoordinates, time);
    }

  }  // namespace testing
}  // namespace openvkl

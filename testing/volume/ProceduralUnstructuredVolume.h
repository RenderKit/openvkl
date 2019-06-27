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
#include "ProceduralStructuredVolume.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/vec.h"

#define INDEX_PREFIX 1
#define CELL_VALUED 1

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <float volumeSamplingFunction(const vec3f &)>
    struct ProceduralUnstructuredVolume : public TestingStructuredVolume
    {
      ProceduralUnstructuredVolume(const vec3i &dimensions,
                                   const vec3f &gridOrigin,
                                   const vec3f &gridSpacing)
          : TestingStructuredVolume(
                "unstructured", dimensions, gridOrigin, gridSpacing, VKL_FLOAT)
      {
      }

      inline float computeProceduralValue(const vec3f &objectCoordinates)
      {
        return volumeSamplingFunction(objectCoordinates);
      }

      std::vector<unsigned char> generateVoxels() override
      {
        return generateVoxels(dimensions);
      }

      std::vector<unsigned char> generateVoxels(vec3i dimensions)
      {
        std::vector<unsigned char> voxels(longProduct(dimensions) *
                                          sizeof(float));
        float *voxelsTyped = (float *)voxels.data();

        auto transformLocalToObject = [&](const vec3f &localCoordinates) {
          return gridOrigin + localCoordinates * gridSpacing;
        };

        for (size_t z = 0; z < dimensions.z; z++) {
          for (size_t y = 0; y < dimensions.y; y++) {
            for (size_t x = 0; x < dimensions.x; x++) {
              size_t index =
                  z * dimensions.y * dimensions.x + y * dimensions.x + x;
              vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
              voxelsTyped[index] = volumeSamplingFunction(objectCoordinates);
            }
          }
        }

        return voxels;
      }

      void generateVKLVolume() override
      {
#if CELL_VALUED
        std::vector<unsigned char> cellValues = generateVoxels(dimensions);
#else
        std::vector<unsigned char> vtxValues = generateVoxels(dimensions + vec3i(1,1,1));
#endif
        std::vector<vec3f> vtxPositions       = generateGrid();
        std::vector<uint32_t> topology        = generateTopology();
        std::vector<uint32_t> cells;
        std::vector<uint8_t> cellType;

        volume = vklNewVolume("unstructured");

        for (int i = 0; i < dimensions.product(); i++) {
#if INDEX_PREFIX
          cells.push_back(i * 9);
#else
          cells.push_back(i * 8);
#endif
          cellType.push_back(VKL_HEXAHEDRON);
        }
        VKLData cellData = vklNewData(cells.size(), VKL_UINT, cells.data());
        vklSetData(volume, "cell.index", cellData);
        vklRelease(cellData);
#if !INDEX_PREFIX
        VKLData celltypeData =
            vklNewData(cellType.size(), VKL_UCHAR, cellType.data());
        vklSetData(volume, "cell.type", celltypeData);
        vklRelease(celltypeData);
#endif

#if CELL_VALUED
        VKLData cellValuesData =
            vklNewData(longProduct(dimensions), VKL_FLOAT, cellValues.data());
        vklSetData(volume, "cell.value", cellValuesData);
        vklRelease(cellValuesData);
#else
        VKLData vtxValuesData =
            vklNewData(longProduct(dimensions + vec3i(1,1,1)), VKL_FLOAT, cellValues.data());
        vklSetData(volume, "cell.value", cellValuesData);
        vklRelease(cellValuesData);
#endif

        VKLData vtxPositionsData =
            vklNewData(vtxPositions.size(), VKL_FLOAT3, vtxPositions.data());
        vklSetData(volume, "vertex.position", vtxPositionsData);
        vklRelease(vtxPositionsData);

        VKLData topologyData =
            vklNewData(topology.size(), VKL_UINT, topology.data());
        vklSetData(volume, "index", topologyData);
        vklRelease(topologyData);

#if INDEX_PREFIX
        vklSetBool(volume, "indexPrefixed", true);
#else
        vklSetBool(volume, "indexPrefixed", false);
#endif

        vklSetBool(volume, "precomputedNormals", true);
        vklSetBool(volume, "hexIterative", false);

        vklCommit(volume);
      }

      std::vector<vec3f> generateGrid()
      {
        std::vector<vec3f> grid(
            (dimensions.x + 1) * (dimensions.y + 1) * (dimensions.z + 1), 0);

        for (size_t z = 0; z <= dimensions.z; z++) {
          for (size_t y = 0; y <= dimensions.y; y++) {
            for (size_t x = 0; x <= dimensions.x; x++) {
              size_t index = z * (dimensions.y + 1) * (dimensions.x + 1) +
                             y * (dimensions.x + 1) + x;
              grid[index] = gridOrigin + gridSpacing * vec3f(x, y, z);
            }
          }
        }
        return grid;
      }

      std::vector<uint32_t> generateTopology()
      {
        std::vector<uint32_t> cells;

        for (size_t z = 0; z < dimensions.z; z++) {
          for (size_t y = 0; y < dimensions.y; y++) {
            for (size_t x = 0; x < dimensions.x; x++) {
              int layerSize = (dimensions.x + 1) * (dimensions.y + 1);
              int offset    = layerSize * z + (dimensions.x + 1) * y + x;
              int offset2   = offset + layerSize;
#if INDEX_PREFIX
              cells.push_back(8);
#endif
              cells.push_back(offset + 0);
              cells.push_back(offset + 1);
              cells.push_back(offset + (dimensions.x + 1) + 1);
              cells.push_back(offset + (dimensions.x + 1));
              cells.push_back(offset2 + 0);
              cells.push_back(offset2 + 1);
              cells.push_back(offset2 + (dimensions.x + 1) + 1);
              cells.push_back(offset2 + (dimensions.x + 1));
            }
          }
        }
        return cells;
      }
    };

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    using WaveletUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<getWaveletValue<float>>;
  }  // namespace testing
}  // namespace openvkl

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

#include "TestingVolume.h"

namespace openvkl {
  namespace testing {

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    struct ProceduralUnstructuredVolume : public TestingVolume
    {
      ProceduralUnstructuredVolume(
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLUnstructuredCellType _primType = VKL_HEXAHEDRON,
          bool _cellValued                  = true,
          bool _indexPrefix                 = true,
          bool _precomputedNormals          = false,
          bool _hexIterative                = false);

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      float computeProceduralValue(const vec3f &objectCoordinates);

     private:
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VKLUnstructuredCellType primType;

      bool cellValued;
      bool indexPrefix;
      bool precomputedNormals;
      bool hexIterative;

      int vtxPerPrimitive(VKLUnstructuredCellType type) const;

      std::vector<unsigned char> generateVoxels(vec3i dimensions);

      void generateVKLVolume() override;

      std::vector<vec3f> generateGrid();

      std::vector<idxType> generateTopology();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline ProceduralUnstructuredVolume<idxType, volumeSamplingFunction>::
        ProceduralUnstructuredVolume(const vec3i &dimensions,
                                     const vec3f &gridOrigin,
                                     const vec3f &gridSpacing,
                                     VKLUnstructuredCellType _primType,
                                     bool _cellValued,
                                     bool _indexPrefix,
                                     bool _precomputedNormals,
                                     bool _hexIterative)
        : dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          primType(_primType),
          cellValued(_cellValued),
          indexPrefix(_indexPrefix),
          precomputedNormals(_precomputedNormals),
          hexIterative(_hexIterative)
    {
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline vec3i
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::getDimensions() const
    {
      return dimensions;
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::getGridOrigin() const
    {
      return gridOrigin;
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::getGridSpacing() const
    {
      return gridSpacing;
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline float ProceduralUnstructuredVolume<idxType, volumeSamplingFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return volumeSamplingFunction(objectCoordinates);
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline int ProceduralUnstructuredVolume<idxType, volumeSamplingFunction>::
        vtxPerPrimitive(VKLUnstructuredCellType type) const
    {
      switch (type) {
      case VKL_TETRAHEDRON:
        return 4;
      case VKL_HEXAHEDRON:
        return 8;
      case VKL_WEDGE:
        return 6;
      case VKL_PYRAMID:
        return 5;
      }
      return 0;
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline std::vector<unsigned char> ProceduralUnstructuredVolume<
        idxType,
        volumeSamplingFunction>::generateVoxels(vec3i dimensions)
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
            voxelsTyped[index]      = volumeSamplingFunction(objectCoordinates);
          }
        }
      }

      return voxels;
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline void
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::generateVKLVolume()
    {
      vec3i valueDimensions = dimensions;
      if (!cellValued)
        valueDimensions += vec3i(1, 1, 1);
      std::vector<unsigned char> values = generateVoxels(valueDimensions);

      std::vector<vec3f> vtxPositions = generateGrid();
      std::vector<idxType> topology   = generateTopology();
      std::vector<idxType> cells;
      std::vector<uint8_t> cellType;

      volume = vklNewVolume("unstructured");

      uint64_t numCells = longProduct(dimensions);
      cells.reserve(numCells);
      cellType.reserve(numCells);

      for (idxType i = 0; i < numCells; i++) {
        cells.push_back(i *
                        (vtxPerPrimitive(primType) + (indexPrefix ? 1 : 0)));
        cellType.push_back(primType);
      }
      VKLData cellData = vklNewData(
          cells.size(),
          std::is_same<idxType, uint32_t>::value ? VKL_UINT : VKL_ULONG,
          cells.data());
      vklSetData(volume, "cell.index", cellData);
      vklRelease(cellData);

      if (!indexPrefix) {
        VKLData celltypeData =
            vklNewData(cellType.size(), VKL_UCHAR, cellType.data());
        vklSetData(volume, "cell.type", celltypeData);
        vklRelease(celltypeData);
      }

      VKLData valuesData =
          vklNewData(longProduct(valueDimensions), VKL_FLOAT, values.data());
      vklSetData(
          volume, cellValued ? "cell.value" : "vertex.value", valuesData);
      vklRelease(valuesData);

      VKLData vtxPositionsData =
          vklNewData(vtxPositions.size(), VKL_FLOAT3, vtxPositions.data());
      vklSetData(volume, "vertex.position", vtxPositionsData);
      vklRelease(vtxPositionsData);

      VKLData topologyData = vklNewData(
          topology.size(),
          std::is_same<idxType, uint32_t>::value ? VKL_UINT : VKL_ULONG,
          topology.data());
      vklSetData(volume, "index", topologyData);
      vklRelease(topologyData);

      vklSetBool(volume, "indexPrefixed", indexPrefix);
      vklSetBool(volume, "precomputedNormals", precomputedNormals);
      vklSetBool(volume, "hexIterative", hexIterative);

      vklCommit(volume);
    }

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline std::vector<vec3f>
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::generateGrid()
    {
      std::vector<vec3f> grid(longProduct(dimensions + vec3i(1, 1, 1)), 0);

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

    template <typename idxType, float volumeSamplingFunction(const vec3f &)>
    inline std::vector<idxType>
    ProceduralUnstructuredVolume<idxType,
                                 volumeSamplingFunction>::generateTopology()
    {
      uint64_t numPerPrim = vtxPerPrimitive(primType);
      if (indexPrefix)
        numPerPrim++;
      std::vector<idxType> cells;
      cells.reserve(longProduct(dimensions) * numPerPrim);

      for (size_t z = 0; z < dimensions.z; z++) {
        for (size_t y = 0; y < dimensions.y; y++) {
          for (size_t x = 0; x < dimensions.x; x++) {
            idxType layerSize = (dimensions.x + 1) * (dimensions.y + 1);
            idxType offset    = layerSize * z + (dimensions.x + 1) * y + x;
            idxType offset2   = offset + layerSize;
            if (indexPrefix)
              cells.push_back(vtxPerPrimitive(primType));
            switch (primType) {
            case VKL_TETRAHEDRON:
              cells.push_back(offset + 0);
              cells.push_back(offset + 1);
              cells.push_back(offset + (dimensions.x + 1) + 0);
              cells.push_back(offset2 + 0);
              break;
            case VKL_HEXAHEDRON:
              cells.push_back(offset + 0);
              cells.push_back(offset + 1);
              cells.push_back(offset + (dimensions.x + 1) + 1);
              cells.push_back(offset + (dimensions.x + 1));
              cells.push_back(offset2 + 0);
              cells.push_back(offset2 + 1);
              cells.push_back(offset2 + (dimensions.x + 1) + 1);
              cells.push_back(offset2 + (dimensions.x + 1));
              break;
            case VKL_WEDGE:
              cells.push_back(offset + 0);
              cells.push_back(offset + 1);
              cells.push_back(offset + (dimensions.x + 1) + 0);
              cells.push_back(offset2 + 0);
              cells.push_back(offset2 + 1);
              cells.push_back(offset2 + (dimensions.x + 1) + 0);
              break;
            case VKL_PYRAMID:
              cells.push_back(offset + 0);
              cells.push_back(offset + 1);
              cells.push_back(offset + (dimensions.x + 1) + 1);
              cells.push_back(offset + (dimensions.x + 1));
              cells.push_back(offset2 + 0);
              break;
            }
          }
        }
      }

      return cells;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline float getConstValue(const vec3f &objectCoordinates)
    {
      return 0.5;
    }

    using WaveletUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getWaveletValue<float>>;

    using ZUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getZValue>;

    using ConstUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getConstValue>;

    using WaveletUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t, getWaveletValue<float>>;

    using ZUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t, getZValue>;

    using ConstUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t, getConstValue>;

  }  // namespace testing
}  // namespace openvkl

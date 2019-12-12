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

#include "TestingVolume.h"
#include "procedural_functions.h"

namespace openvkl {
  namespace testing {

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &) = gradientNotImplemented>
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

      range1f getComputedValueRange() const override;

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      float computeProceduralValue(const vec3f &objectCoordinates);

      vec3f computeProceduralGradient(const vec3f &objectCoordinates);

     private:
      range1f computedValueRange = range1f(ospcommon::math::empty);

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

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline ProceduralUnstructuredVolume<idxType,
                                        samplingFunction,
                                        gradientFunction>::
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

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline range1f
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        getComputedValueRange() const
    {
      if (computedValueRange.empty()) {
        throw std::runtime_error(
            "computedValueRange only available after VKL volume is generated");
      }

      return computedValueRange;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3i ProceduralUnstructuredVolume<idxType,
                                              samplingFunction,
                                              gradientFunction>::getDimensions()
        const
    {
      return dimensions;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f ProceduralUnstructuredVolume<idxType,
                                              samplingFunction,
                                              gradientFunction>::getGridOrigin()
        const
    {
      return gridOrigin;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        getGridSpacing() const
    {
      return gridSpacing;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline float
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        computeProceduralValue(const vec3f &objectCoordinates)
    {
      return samplingFunction(objectCoordinates);
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        computeProceduralGradient(const vec3f &objectCoordinates)
    {
      return gradientFunction(objectCoordinates);
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline int
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
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

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<unsigned char>
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateVoxels(vec3i dimensions)
    {
      std::vector<unsigned char> voxels(dimensions.long_product() *
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
            voxelsTyped[index]      = samplingFunction(objectCoordinates);
          }
        }
      }

      return voxels;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline void
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateVKLVolume()
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

      uint64_t numCells = dimensions.long_product();
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
          vklNewData(valueDimensions.long_product(), VKL_FLOAT, values.data());
      vklSetData(
          volume, cellValued ? "cell.value" : "vertex.value", valuesData);
      vklRelease(valuesData);

      VKLData vtxPositionsData =
          vklNewData(vtxPositions.size(), VKL_VEC3F, vtxPositions.data());
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

      computedValueRange = computeValueRange(
          VKL_FLOAT, values.data(), valueDimensions.long_product());
    }

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<vec3f>
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateGrid()
    {
      std::vector<vec3f> grid((dimensions + vec3i(1, 1, 1)).long_product(), 0);

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

    template <typename idxType,
              float samplingFunction(const vec3f &),
              vec3f gradientFunction(const vec3f &)>
    inline std::vector<idxType>
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateTopology()
    {
      uint64_t numPerPrim = vtxPerPrimitive(primType);
      if (indexPrefix)
        numPerPrim++;
      std::vector<idxType> cells;
      cells.reserve(dimensions.long_product() * numPerPrim);

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

    using WaveletUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t,
                                     getWaveletValue<float>,
                                     getWaveletGradient>;

    using ZUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getZValue, getZGradient>;

    using ConstUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getConstValue, getConstGradient>;

    using XYZUnstructuredProceduralVolume =
        ProceduralUnstructuredVolume<uint32_t, getXYZValue, getXYZGradient>;

    using WaveletUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t,
                                     getWaveletValue<float>,
                                     getWaveletGradient>;

    using ZUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t, getZValue, getZGradient>;

    using ConstUnstructuredProceduralVolume64 =
        ProceduralUnstructuredVolume<uint64_t, getConstValue, getConstGradient>;

  }  // namespace testing
}  // namespace openvkl

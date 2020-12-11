// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ProceduralVolume.h"
#include "TestingVolume.h"
#include "procedural_functions.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace testing {

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float) = gradientNotImplemented>
    struct ProceduralUnstructuredVolume : public TestingVolume,
                                          public ProceduralVolume
    {
      ProceduralUnstructuredVolume(
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLUnstructuredCellType primType       = VKL_HEXAHEDRON,
          bool cellValued                        = true,
          bool indexPrefix                       = true,
          bool precomputedNormals                = false,
          bool hexIterative                      = false,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      range1f getComputedValueRange() const override;

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

     private:
      range1f computedValueRange = range1f(rkcommon::math::empty);

      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VKLUnstructuredCellType primType;

      bool cellValued;
      bool indexPrefix;
      bool precomputedNormals;
      bool hexIterative;

      // these flags are used for `cell.data` or `vertex.data` data objects as
      // appropriate
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;

      // data may need to be retained for shared data buffers (`cell.data` or
      // `vertex.data`)
      std::vector<unsigned char> values;

      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override;

      vec3f computeProceduralGradientImpl(
          const vec3f &objectCoordinates, float time) const override;

      int vtxPerPrimitive(VKLUnstructuredCellType type) const;

      std::vector<unsigned char> generateVoxels(vec3i dimensions);

      void generateVKLVolume() override;

      std::vector<vec3f> generateGrid();

      std::vector<idxType> generateTopology();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline ProceduralUnstructuredVolume<idxType,
                                        samplingFunction,
                                        gradientFunction>::
        ProceduralUnstructuredVolume(const vec3i &dimensions,
                                     const vec3f &gridOrigin,
                                     const vec3f &gridSpacing,
                                     VKLUnstructuredCellType primType,
                                     bool cellValued,
                                     bool indexPrefix,
                                     bool precomputedNormals,
                                     bool hexIterative,
                                     VKLDataCreationFlags dataCreationFlags,
                                     size_t byteStride)
        : ProceduralVolume(false),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          primType(primType),
          cellValued(cellValued),
          indexPrefix(indexPrefix),
          precomputedNormals(precomputedNormals),
          hexIterative(hexIterative),
          dataCreationFlags(dataCreationFlags),
          byteStride(byteStride)
    {
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
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
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3i ProceduralUnstructuredVolume<idxType,
                                              samplingFunction,
                                              gradientFunction>::getDimensions()
        const
    {
      return dimensions;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f ProceduralUnstructuredVolume<idxType,
                                              samplingFunction,
                                              gradientFunction>::getGridOrigin()
        const
    {
      return gridOrigin;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        getGridSpacing() const
    {
      return gridSpacing;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline float
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        computeProceduralValueImpl(const vec3f &objectCoordinates,
                                   float time) const
    {
      return samplingFunction(objectCoordinates, time);
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        computeProceduralGradientImpl(const vec3f &objectCoordinates, float time) const
    {
      return gradientFunction(objectCoordinates, time);
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
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
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline std::vector<unsigned char>
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateVoxels(vec3i dimensions)
    {
      if (byteStride == 0) {
        byteStride = sizeof(float);
      }

      if (byteStride < sizeof(float)) {
        throw std::runtime_error("byteStride < sizeof(float)");
      }

      std::vector<unsigned char> voxels(dimensions.long_product() * byteStride);

      auto transformLocalToObject = [&](const vec3f &localCoordinates) {
        return gridOrigin + localCoordinates * gridSpacing;
      };

      rkcommon::tasking::parallel_for(dimensions.z, [&](int z) {
        for (size_t y = 0; y < dimensions.y; y++) {
          for (size_t x = 0; x < dimensions.x; x++) {
            size_t index =
                size_t(z) * dimensions.y * dimensions.x + y * dimensions.x + x;
            float *voxelTyped = (float *)(voxels.data() + index * byteStride);
            vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
            *voxelTyped             = samplingFunction(objectCoordinates, 0.f);
          }
        }
      });

      return voxels;
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralUnstructuredVolume<idxType, samplingFunction, gradientFunction>::
        generateVKLVolume()
    {
      vec3i valueDimensions = dimensions;
      if (!cellValued)
        valueDimensions += vec3i(1, 1, 1);
      values = generateVoxels(valueDimensions);

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

      VKLData valuesData = vklNewData(valueDimensions.long_product(),
                                      VKL_FLOAT,
                                      values.data(),
                                      dataCreationFlags,
                                      byteStride);
      vklSetData(volume, cellValued ? "cell.data" : "vertex.data", valuesData);
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

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<unsigned char>().swap(values);
      }
    }

    template <typename idxType,
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
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
              float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
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

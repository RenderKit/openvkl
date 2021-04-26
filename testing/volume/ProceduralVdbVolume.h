// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/AffineSpace.h>
#include "ProceduralVolume.h"
#include "TestingVolume.h"
#include "openvkl/vdb.h"
#include "openvkl/vdb_util/VdbVolumeBuffers.h"

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    inline int getNumLeaves(int res)
    {
      const int shift = vklVdbLevelLogRes(vklVdbNumLevels() - 1);
      int numLeaves   = res >> shift;
      return numLeaves + ((numLeaves << shift) < res);
    }

    // allows handling of generic procedural structured volumes without
    // knowledge of template parameters
    struct ProceduralVdbVolumeBase : public TestingVolume,
                                     public ProceduralVolume
    {
      ProceduralVdbVolumeBase(const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing,
                              VKLDataType voxelType,
                              const TemporalConfig &temporalConfig)
          : dimensions(dimensions),
            gridOrigin(gridOrigin),
            gridSpacing(gridSpacing),
            voxelType(voxelType),
            ProceduralVolume(false)
      {
      }

      vec3i getDimensions() const
      {
        return dimensions;
      }
      vec3f getGridOrigin() const
      {
        return gridOrigin;
      }
      vec3f getGridSpacing() const
      {
        return gridSpacing;
      }
      VKLDataType getVoxelType() const
      {
        return voxelType;
      }
      const TemporalConfig &getTemporalConfig() const
      {
        return temporalConfig;
      }

      virtual vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const = 0;

     protected:
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLDataType voxelType;
      TemporalConfig temporalConfig;
    };

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float) =
                  gradientNotImplemented>
    struct ProceduralVdbVolume : public ProceduralVdbVolumeBase
    {
      using Buffers = vdb_util::VdbVolumeBuffers;

      ProceduralVdbVolume()                            = default;
      ProceduralVdbVolume(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume &operator=(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume(ProceduralVdbVolume &&)                 = default;
      ProceduralVdbVolume &operator=(ProceduralVdbVolume &&) = default;
      ~ProceduralVdbVolume()                                 = default;

      ProceduralVdbVolume(
          VKLDevice device,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLFilter filter                       = VKL_FILTER_TRILINEAR,
          const TemporalConfig &temporalConfig   = TemporalConfig(),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      range1f getComputedValueRange() const override;

      vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const override;

     protected:
      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override;

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          float time) const override;

      void generateVKLVolume(VKLDevice device) override;

      void addTemporallyConstantLeaf(int x, int y, int z, size_t byteStride);

      void addTemporallyStructuredLeaf(int x,
                                       int y,
                                       int z,
                                       size_t byteStride,
                                       const TemporalConfig &temporalConfig);

      void addTemporallyUnstructuredLeaf(int x,
                                         int y,
                                         int z,
                                         size_t byteStride,
                                         const TemporalConfig &temporalConfig);

     private:
      std::unique_ptr<Buffers> buffers;
      range1f valueRange;
      VKLFilter filter;
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;

      // leaf and motion blur data may need to be retained for shared data
      // buffers
      std::vector<std::vector<unsigned char>> leaves;
      std::vector<std::vector<uint32_t>> indices;
      std::vector<std::vector<float>> times;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        addTemporallyConstantLeaf(int x, int y, int z, size_t byteStride)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);

      // Buffer for leaf data.
      leaves.emplace_back(
          std::vector<unsigned char>(numLeafVoxels * byteStride));
      std::vector<unsigned char> &leaf = leaves.back();

      range1f leafValueRange;
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      for (uint32_t vx = 0; vx < leafRes; ++vx) {
        for (uint32_t vy = 0; vy < leafRes; ++vy) {
          for (uint32_t vz = 0; vz < leafRes; ++vz) {
            // Note: column major data!
            const uint64_t idx = static_cast<uint64_t>(vx) * leafRes * leafRes +
                                 static_cast<uint64_t>(vy) * leafRes +
                                 static_cast<uint64_t>(vz);

            const vec3f samplePosIndex =
                vec3f(nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
            const vec3f samplePosObject =
                transformLocalToObjectCoordinates(samplePosIndex);

            const VOXEL_TYPE fieldValue =
                samplingFunction(samplePosObject, 0.f);

            VOXEL_TYPE *leafValueTyped =
                (VOXEL_TYPE *)(leaf.data() + idx * byteStride);
            *leafValueTyped = fieldValue;

            leafValueRange.extend(fieldValue);
          }
        }
      }
      // Skip empty nodes.
      if (leafValueRange.lower != 0.f || leafValueRange.upper != 0.f) {
        // We compress constant areas of space into tiles.
        // We also copy all leaf data so that we do not have to
        // explicitly store it.
        if (std::fabs(leafValueRange.upper - leafValueRange.lower) <
            std::fabs(leafValueRange.upper) *
                std::numeric_limits<float>::epsilon()) {
          buffers->addTile(leafLevel, nodeOrigin, {&leafValueRange.upper});
        } else
          buffers->addConstant(
              leafLevel,
              nodeOrigin,
              {leaf.data()},
              dataCreationFlags,
              {byteStride},
              static_cast<uint32_t>(temporalConfig.sampleTime.size()));
      }
      valueRange.extend(leafValueRange);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        leaves.clear();
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        addTemporallyStructuredLeaf(int x,
                                    int y,
                                    int z,
                                    size_t byteStride,
                                    const TemporalConfig &temporalConfig)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);
      const uint32_t numTimesteps =
          static_cast<uint32_t>(temporalConfig.sampleTime.size());

      // Buffer for leaf data.
      leaves.emplace_back(std::vector<unsigned char>(
          numLeafVoxels * byteStride * numTimesteps));
      std::vector<unsigned char> &leaf = leaves.back();

      range1f leafValueRange;
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      for (uint32_t vx = 0; vx < leafRes; ++vx) {
        for (uint32_t vy = 0; vy < leafRes; ++vy) {
          for (uint32_t vz = 0; vz < leafRes; ++vz) {
            for (uint32_t vt = 0; vt < numTimesteps; ++vt) {
              // Note: column major data!
              const uint64_t idx =
                  static_cast<uint64_t>(vx) * leafRes * leafRes * numTimesteps +
                  static_cast<uint64_t>(vy) * leafRes * numTimesteps +
                  static_cast<uint64_t>(vz) * numTimesteps +
                  static_cast<uint64_t>(vt);

              const vec3f samplePosIndex = vec3f(
                  nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
              const vec3f samplePosObject =
                  transformLocalToObjectCoordinates(samplePosIndex);

              const VOXEL_TYPE fieldValue = samplingFunction(
                  samplePosObject, temporalConfig.sampleTime[vt]);

              VOXEL_TYPE *leafValueTyped =
                  (VOXEL_TYPE *)(leaf.data() + idx * byteStride);
              *leafValueTyped = fieldValue;

              leafValueRange.extend(fieldValue);
            }
          }
        }
      }
      // Skip empty nodes.
      if (leafValueRange.lower != 0.f || leafValueRange.upper != 0.f) {
        buffers->addConstant(
            leafLevel,
            nodeOrigin,
            {leaf.data()},
            dataCreationFlags,
            {byteStride},
            static_cast<uint32_t>(temporalConfig.sampleTime.size()));
      }
      valueRange.extend(leafValueRange);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        leaves.clear();
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        addTemporallyUnstructuredLeaf(int x,
                                      int y,
                                      int z,
                                      size_t byteStride,
                                      const TemporalConfig &temporalConfig)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);
      const uint32_t numTimesteps =
          static_cast<uint32_t>(temporalConfig.sampleTime.size());

      // Buffer for leaf data.
      leaves.emplace_back(std::vector<unsigned char>(
          numLeafVoxels * byteStride * numTimesteps));
      times.emplace_back(std::vector<float>(numLeafVoxels * numTimesteps));
      indices.emplace_back(std::vector<uint32_t>(numLeafVoxels + 1));
      std::vector<unsigned char> &leaf = leaves.back();
      std::vector<uint32_t> &tuvIndex  = indices.back();
      std::vector<float> &time         = times.back();

      range1f leafValueRange;
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      for (uint32_t vx = 0; vx < leafRes; ++vx) {
        for (uint32_t vy = 0; vy < leafRes; ++vy) {
          for (uint32_t vz = 0; vz < leafRes; ++vz) {
            // Note: column major data!
            const uint64_t voxelIdx =
                static_cast<uint64_t>(vx) * leafRes * leafRes +
                static_cast<uint64_t>(vy) * leafRes + static_cast<uint64_t>(vz);
            const uint64_t idx64 = voxelIdx * numTimesteps;
            if (!((idx64+numTimesteps) < (1ul << 32))) {
              throw std::runtime_error(
                  "Too many time steps on unstructured volume.");
            }
            const uint32_t idx = static_cast<uint32_t>(idx64);
            tuvIndex[voxelIdx] = idx;
            tuvIndex[voxelIdx + 1] = idx + numTimesteps;
            const vec3f samplePosIndex =
                vec3f(nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
            const vec3f samplePosObject =
                transformLocalToObjectCoordinates(samplePosIndex);
            for (uint32_t vt = 0; vt < numTimesteps; ++vt) {
              const VOXEL_TYPE fieldValue = samplingFunction(
                  samplePosObject, temporalConfig.sampleTime[vt]);
              time[idx + vt] = temporalConfig.sampleTime[vt];

              VOXEL_TYPE *leafValueTyped =
                  (VOXEL_TYPE *)(leaf.data() + (idx + vt) * byteStride);
              *leafValueTyped = fieldValue;
              leafValueRange.extend(fieldValue);
            }
          }
        }
      }
      // Skip empty nodes.
      if (leafValueRange.lower != 0.f || leafValueRange.upper != 0.f) {
        buffers->addConstant(leafLevel,
                             nodeOrigin,
                             {leaf.data()},
                             dataCreationFlags,
                             {byteStride},
                             0,
                             tuvIndex.size(),
                             tuvIndex.data(),
                             time.data());
      }
      valueRange.extend(leafValueRange);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        leaves.clear();
        indices.clear();
        times.clear();
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        ProceduralVdbVolume(VKLDevice device,
                            const vec3i &dimensions,
                            const vec3f &gridOrigin,
                            const vec3f &gridSpacing,
                            VKLFilter filter,
                            const TemporalConfig &temporalConfig,
                            VKLDataCreationFlags dataCreationFlags,
                            size_t byteStride)
        : ProceduralVdbVolumeBase(dimensions,
                                  gridOrigin,
                                  gridSpacing,
                                  getVKLDataType<VOXEL_TYPE>(),
                                  temporalConfig),
          buffers(new Buffers(device, {getVKLDataType<VOXEL_TYPE>()})),
          filter(filter),
          dataCreationFlags(dataCreationFlags),
          byteStride(byteStride)
    {
      if (byteStride == 0) {
        byteStride = sizeOfVKLDataType(voxelType);
      }

      if (byteStride < sizeOfVKLDataType(voxelType)) {
        throw std::runtime_error("byteStride must be >= size of voxel type");
      }

      buffers->setIndexToObject(gridSpacing.x,
                                0,
                                0,
                                0,
                                gridSpacing.y,
                                0,
                                0,
                                0,
                                gridSpacing.z,
                                gridOrigin.x,
                                gridOrigin.y,
                                gridOrigin.z);

      // The number of leaf nodes, in each dimension.
      const vec3i numLeafNodesIn(getNumLeaves(dimensions.x),
                                 getNumLeaves(dimensions.y),
                                 getNumLeaves(dimensions.z));

      const size_t numLeafNodes = numLeafNodesIn.x *
                                  static_cast<size_t>(numLeafNodesIn.y) *
                                  numLeafNodesIn.z;

      buffers->reserve(numLeafNodes);

      valueRange = range1f();
      for (int x = 0; x < numLeafNodesIn.x; ++x) {
        for (int y = 0; y < numLeafNodesIn.y; ++y) {
          for (int z = 0; z < numLeafNodesIn.z; ++z) {
            switch (temporalConfig.type) {
            case TemporalConfig::Constant:
              addTemporallyConstantLeaf(x, y, z, byteStride);
              break;
            case TemporalConfig::Structured:
              addTemporallyStructuredLeaf(x, y, z, byteStride, temporalConfig);
              break;
            case TemporalConfig::Unstructured:
              addTemporallyUnstructuredLeaf(
                  x, y, z, byteStride, temporalConfig);
              break;
            default:
              assert(false);
              break;
            }
          }
        }
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline range1f
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        getComputedValueRange() const
    {
      return valueRange;
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        transformLocalToObjectCoordinates(const vec3f &localCoordinates) const
    {
      return gridSpacing * localCoordinates + gridOrigin;
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline float
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralValueImpl(const vec3f &objectCoordinates,
                                   float time) const
    {
      return samplingFunction(objectCoordinates, time);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                      float time) const
    {
      return gradientFunction(objectCoordinates, time);
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        generateVKLVolume(VKLDevice device)
    {
      if (buffers) {
        release();

        if (device != buffers->getVKLDevice()) {
          throw std::runtime_error(
              "specified device not compatible with VdbVolumeBuffers device");
        }

        volume = buffers->createVolume(filter);
      }
    }

    // half procedural volumes
    using WaveletVdbVolumeHalf =
        ProceduralVdbVolume<half_float::half,
                            getWaveletValue<half_float::half>,
                            getWaveletGradient>;

    using XYZVdbVolumeHalf = ProceduralVdbVolume<half_float::half,
                                                 getXYZValue<half_float::half>,
                                                 getXYZGradient>;

    using XVdbVolumeHalf = ProceduralVdbVolume<half_float::half,
                                               getXValue<half_float::half>,
                                               getXGradient>;

    using YVdbVolumeHalf = ProceduralVdbVolume<half_float::half,
                                               getYValue<half_float::half>,
                                               getYGradient>;

    using ZVdbVolumeHalf = ProceduralVdbVolume<half_float::half,
                                               getZValue<half_float::half>,
                                               getZGradient>;

    using SphereVdbVolumeHalf =
        ProceduralVdbVolume<half_float::half,
                            getRotatingSphereValue<half_float::half>,
                            getRotatingSphereGradient>;

    // float procedural volumes

    using WaveletVdbVolumeFloat =
        ProceduralVdbVolume<float, getWaveletValue<float>, getWaveletGradient>;

    using XYZVdbVolumeFloat =
        ProceduralVdbVolume<float, getXYZValue<float>, getXYZGradient>;

    using XVdbVolumeFloat =
        ProceduralVdbVolume<float, getXValue<float>, getXGradient>;

    using YVdbVolumeFloat =
        ProceduralVdbVolume<float, getYValue<float>, getYGradient>;

    using ZVdbVolumeFloat =
        ProceduralVdbVolume<float, getZValue<float>, getZGradient>;

    using SphereVdbVolumeFloat =
        ProceduralVdbVolume<float,
                            getRotatingSphereValue<float>,
                            getRotatingSphereGradient>;

  }  // namespace testing
}  // namespace openvkl

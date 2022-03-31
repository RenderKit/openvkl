// Copyright 2020-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/AffineSpace.h>
#include "ProceduralVolume.h"
#include "TestingVolume.h"
#include "openvkl/utility/temporal_compression/douglas_peucker.h"
#include "openvkl/utility/vdb/VdbVolumeBuffers.h"
#include "openvkl/vdb.h"

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
      using Buffers = utility::vdb::VdbVolumeBuffers;

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
          bool repackNodes                       = true,
          const TemporalConfig &temporalConfig   = TemporalConfig(),
          uint32_t numAttributes                 = 1,
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

      void addTemporallyConstantLeaf(
          int x, int y, int z, size_t byteStride, uint32_t numAttributes);

      void addTemporallyStructuredLeaf(int x,
                                       int y,
                                       int z,
                                       size_t byteStride,
                                       const TemporalConfig &temporalConfig,
                                       uint32_t numAttributes);

      void addTemporallyUnstructuredLeaf(int x,
                                         int y,
                                         int z,
                                         size_t byteStride,
                                         const TemporalConfig &temporalConfig,
                                         uint32_t numAttributes);

      void addLeaf(std::vector<void *> &ptrs,
                   std::vector<size_t> &byteStrides,
                   std::vector<range1f> &leafValueRanges,
                   const vec3i &nodeOrigin,
                   uint32_t numTimesteps = 0);

     private:
      std::unique_ptr<Buffers> buffers;
      range1f valueRange;
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;
      uint32_t numAttributes;

      // leaf and motion blur data may need to be retained for shared data
      // buffers
      std::vector<std::vector<unsigned char>> leaves;
      std::vector<std::vector<uint32_t>> indices;
      std::vector<std::vector<float>> times;

      // Some memory stats.
      size_t bytesUncompressed{0};
      size_t bytesCompressed{0};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        addLeaf(std::vector<void *> &ptrs,
                std::vector<size_t> &byteStrides,
                std::vector<range1f> &leafValueRanges,
                const vec3i &nodeOrigin,
                uint32_t numTimesteps)
    {
      bool nodeEmpty = true;
      for (size_t a = 0; a < ptrs.size(); ++a) {
        if (leafValueRanges[a].lower != 0.f ||
            leafValueRanges[a].upper != 0.f) {
          nodeEmpty = false;
          break;
        }
      }

      // Skip empty nodes.
      if (!nodeEmpty) {
        const uint32_t leafLevel   = vklVdbNumLevels() - 1;
        const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);
        // We compress constant areas of space into tiles.
        // We also copy all leaf data so that we do not have to
        // explicitly store it.
        if (numTimesteps == 0 && ptrs.size() == 1 &&
            std::fabs(leafValueRanges[0].upper - leafValueRanges[0].lower) <
                std::fabs(leafValueRanges[0].upper) *
                    std::numeric_limits<float>::epsilon()) {
          buffers->addTile(leafLevel, nodeOrigin, {&leafValueRanges[0].upper});
        } else {
          buffers->addConstant(leafLevel,
                               nodeOrigin,
                               ptrs,
                               dataCreationFlags,
                               byteStrides,
                               numTimesteps);
        }

        for (const auto &vr : leafValueRanges) {
          valueRange.extend(vr);
        }
      }
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void
    ProceduralVdbVolume<VOXEL_TYPE, samplingFunction, gradientFunction>::
        addTemporallyConstantLeaf(
            int x, int y, int z, size_t byteStride, uint32_t numAttributes)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);

      // Buffer for leaf data.
      leaves.emplace_back(std::vector<unsigned char>(
          numLeafVoxels * numAttributes * byteStride));
      std::vector<unsigned char> &leaf = leaves.back();
      bytesUncompressed += leaf.size();
      bytesCompressed += leaf.size();  // We do not compress here.

      std::vector<range1f> leafValueRanges(numAttributes);
      std::vector<void *> ptrs(numAttributes, nullptr);
      std::vector<size_t> byteStrides(numAttributes, byteStride);
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      for (uint32_t va = 0; va < numAttributes; ++va) {
        ptrs[va] = leaf.data() + static_cast<uint64_t>(va) * leafRes * leafRes *
                                     leafRes * byteStride;
        for (uint32_t vx = 0; vx < leafRes; ++vx) {
          for (uint32_t vy = 0; vy < leafRes; ++vy) {
            for (uint32_t vz = 0; vz < leafRes; ++vz) {
              // Note: column major data!
              const uint64_t idx =
                  static_cast<uint64_t>(va) * leafRes * leafRes * leafRes +
                  static_cast<uint64_t>(vx) * leafRes * leafRes +
                  static_cast<uint64_t>(vy) * leafRes +
                  static_cast<uint64_t>(vz);

              const vec3f samplePosIndex = vec3f(
                  nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
              const vec3f samplePosObject = transformLocalToObjectCoordinates(
                  static_cast<float>(va + 1) * samplePosIndex);

              const VOXEL_TYPE fieldValue =
                  samplingFunction(samplePosObject, 0.f);

              VOXEL_TYPE *leafValueTyped =
                  (VOXEL_TYPE *)(leaf.data() + idx * byteStride);
              *leafValueTyped = fieldValue;

              leafValueRanges[va].extend(fieldValue);
            }
          }
        }
      }

      addLeaf(ptrs, byteStrides, leafValueRanges, nodeOrigin, 0);

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
                                    const TemporalConfig &temporalConfig,
                                    uint32_t numAttributes)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);
      const uint32_t numTimesteps =
          static_cast<uint32_t>(temporalConfig.sampleTime.size());

      // Buffer for leaf data.
      leaves.emplace_back(std::vector<unsigned char>(
          numLeafVoxels * numAttributes * byteStride * numTimesteps));
      std::vector<unsigned char> &leaf = leaves.back();
      bytesUncompressed += leaf.size();
      bytesCompressed += leaf.size();  // We do not compress here.

      std::vector<range1f> leafValueRanges(numAttributes);
      std::vector<void *> ptrs(numAttributes, nullptr);
      std::vector<size_t> byteStrides(numAttributes, byteStride);
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      for (uint32_t va = 0; va < numAttributes; ++va) {
        ptrs[va] = leaf.data() + static_cast<uint64_t>(va) * leafRes * leafRes *
                                     leafRes * numTimesteps * byteStride;
        for (uint32_t vx = 0; vx < leafRes; ++vx) {
          for (uint32_t vy = 0; vy < leafRes; ++vy) {
            for (uint32_t vz = 0; vz < leafRes; ++vz) {
              for (uint32_t vt = 0; vt < numTimesteps; ++vt) {
                // Note: z major data!
                const uint64_t idx =
                    static_cast<uint64_t>(va) * leafRes * leafRes * leafRes *
                        numTimesteps +
                    static_cast<uint64_t>(vx) * leafRes * leafRes *
                        numTimesteps +
                    static_cast<uint64_t>(vy) * leafRes * numTimesteps +
                    static_cast<uint64_t>(vz) * numTimesteps +
                    static_cast<uint64_t>(vt);

                const vec3f samplePosIndex = vec3f(
                    nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
                const vec3f samplePosObject = transformLocalToObjectCoordinates(
                    static_cast<float>(va + 1) * samplePosIndex);

                const VOXEL_TYPE fieldValue = samplingFunction(
                    samplePosObject, temporalConfig.sampleTime[vt]);

                VOXEL_TYPE *leafValueTyped =
                    (VOXEL_TYPE *)(leaf.data() + idx * byteStride);
                *leafValueTyped = fieldValue;

                leafValueRanges[va].extend(fieldValue);
              }
            }
          }
        }
      }

      addLeaf(ptrs,
              byteStrides,
              leafValueRanges,
              nodeOrigin,
              static_cast<uint32_t>(temporalConfig.sampleTime.size()));

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
                                      const TemporalConfig &temporalConfig,
                                      uint32_t numAttributes)
    {
      const uint32_t leafLevel   = vklVdbNumLevels() - 1;
      const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
      const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);
      const size_t numTimesteps  = temporalConfig.sampleTime.size();

      // Buffers for leaf data.
      leaves.emplace_back();
      times.emplace_back();
      indices.emplace_back();

      std::vector<unsigned char> &leaf = leaves.back();
      std::vector<uint32_t> &tuvIndex  = indices.back();
      std::vector<float> &time         = times.back();

      tuvIndex.resize(numLeafVoxels + 1);
      leaf.resize(numLeafVoxels * numTimesteps * byteStride);
      time.resize(numLeafVoxels * numTimesteps);

      // Buffers for a single voxel, which we will compress.
      std::vector<VOXEL_TYPE> singleVoxelSamples(numTimesteps);
      std::vector<float> singleVoxelTimes(numTimesteps);
      bytesUncompressed += leaf.size();

      range1f leafValueRange;
      const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
      uint32_t lastEnd = 0;
      for (uint32_t vx = 0; vx < leafRes; ++vx) {
        for (uint32_t vy = 0; vy < leafRes; ++vy) {
          for (uint32_t vz = 0; vz < leafRes; ++vz) {
            const vec3f samplePosIndex =
                vec3f(nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
            const vec3f samplePosObject =
                transformLocalToObjectCoordinates(samplePosIndex);
            for (uint32_t vt = 0; vt < numTimesteps; ++vt) {
              singleVoxelTimes[vt] = temporalConfig.sampleTime[vt];
              singleVoxelSamples[vt] =
                  samplingFunction(samplePosObject, singleVoxelTimes[vt]);
            }

            size_t compressedNumSamples = numTimesteps;
            if (temporalConfig.useTemporalCompression) {
              compressedNumSamples =
                  openvkl::utility::temporal_compression::douglas_peucker(
                      numTimesteps,
                      singleVoxelSamples.data(),
                      singleVoxelTimes.data(),
                      temporalConfig.temporalCompressionThreshold);
            }
            assert(compressedNumSamples > 0);

            const uint64_t lastEnd64 = lastEnd + compressedNumSamples;
            if (!(lastEnd64 < (((uint64_t)1) << 32))) {
              throw std::runtime_error(
                  "Too many time steps on temporally unstructured volume.");
            }
            float lastTime = -1.f;
            for (size_t vt = 0; vt < compressedNumSamples; ++vt) {
              VOXEL_TYPE *leafValueTyped = reinterpret_cast<VOXEL_TYPE *>(
                  leaf.data() + (lastEnd + vt) * byteStride);
              *leafValueTyped       = singleVoxelSamples[vt];
              time.at(lastEnd + vt) = singleVoxelTimes[vt];
              leafValueRange.extend(*leafValueTyped);
              lastTime = time.at(lastEnd + vt);
            }

            // Note: column major data!
            const uint64_t voxelIdx =
                static_cast<uint64_t>(vx) * leafRes * leafRes +
                static_cast<uint64_t>(vy) * leafRes + static_cast<uint64_t>(vz);

            tuvIndex.at(voxelIdx)     = lastEnd;
            lastEnd                   = static_cast<uint32_t>(lastEnd64);
            tuvIndex.at(voxelIdx + 1) = lastEnd;
          }
        }
      }
      leaf.resize(lastEnd * byteStride);
      time.resize(lastEnd);
      bytesCompressed += leaf.size();

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
        valueRange.extend(leafValueRange);
      }

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
                            bool repackNodes,
                            const TemporalConfig &temporalConfig,
                            uint32_t numAttributes,
                            VKLDataCreationFlags dataCreationFlags,
                            size_t byteStride)
        : ProceduralVdbVolumeBase(dimensions,
                                  gridOrigin,
                                  gridSpacing,
                                  getVKLDataType<VOXEL_TYPE>(),
                                  temporalConfig),
          buffers(nullptr),
          numAttributes(std::max<uint32_t>(numAttributes, 1u)),
          dataCreationFlags(dataCreationFlags),
          byteStride(byteStride),
          bytesUncompressed(0),
          bytesCompressed(0)
    {
      if (byteStride == 0) {
        byteStride = sizeOfVKLDataType(voxelType);
      }

      if (byteStride < sizeOfVKLDataType(voxelType)) {
        throw std::runtime_error("byteStride must be >= size of voxel type");
      }

      std::vector<VKLDataType> voxelTypes(numAttributes,
                                          getVKLDataType<VOXEL_TYPE>());
      buffers = rkcommon::make_unique<Buffers>(device, voxelTypes, repackNodes);

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

      // this may over-allocate leaf node storage, if we happen to have tiles.
      buffers->reserve(numLeafNodes, 0);

      valueRange = range1f();
      for (int x = 0; x < numLeafNodesIn.x; ++x) {
        for (int y = 0; y < numLeafNodesIn.y; ++y) {
          for (int z = 0; z < numLeafNodesIn.z; ++z) {
            switch (temporalConfig.type) {
            case TemporalConfig::Constant:
              addTemporallyConstantLeaf(x, y, z, byteStride, numAttributes);
              break;
            case TemporalConfig::Structured:
              addTemporallyStructuredLeaf(
                  x, y, z, byteStride, temporalConfig, numAttributes);
              break;
            case TemporalConfig::Unstructured:
              if (numAttributes == 1) {
                addTemporallyUnstructuredLeaf(
                    x, y, z, byteStride, temporalConfig, numAttributes);
              } else {
                addTemporallyStructuredLeaf(
                    x, y, z, byteStride, temporalConfig, numAttributes);
              }
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

        volume = buffers->createVolume();
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

// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
// openvkl
#include "TestingVolume.h"
// rkcommon
#include "rkcommon/math/range.h"

namespace openvkl {
  namespace testing {

    struct TemporalConfig
    {
      TemporalConfig(size_t numTimesteps            = 1,
                     std::vector<float> timeSamples = std::vector<float>())
          : numTimesteps(numTimesteps), timeSamples(timeSamples)
      {
        if (numTimesteps < 1) {
          throw std::runtime_error("numTimesteps must be >= 1");
        }

        if (!timeSamples.empty() && numTimesteps != timeSamples.size()) {
          throw std::runtime_error(
              "timeSamples provided with incorrect length");
        }
      }

      bool hasStructuredTime() const
      {
        return numTimesteps > 1 && timeSamples.empty();
      }

      bool hasUnstructuredTime() const
      {
        return numTimesteps > 1 && numTimesteps == timeSamples.size();
      }

      bool hasTime() const
      {
        return hasStructuredTime() || hasUnstructuredTime();
      }

      int getStructuredNumTimesteps() const
      {
        return hasStructuredTime() ? numTimesteps : 0;
      }

      VKLData generateUnstructuredIndicesData(size_t numVoxels) const
      {
        if (hasUnstructuredTime()) {
          if (numVoxels * numTimesteps <
              size_t(std::numeric_limits<uint32_t>::max())) {
            std::vector<uint32_t> indices(numVoxels + 1);

            // this would be a std::exclusive_scan() in C++17
            indices[0] = 0;
            for (size_t j = 1; j < indices.size(); j++) {
              indices[j] = indices[j - 1] + numTimesteps;
            }

            assert(indices.back() == numVoxels * numTimesteps);

            return vklNewData(indices.size(), VKL_UINT, indices.data());
          } else {
            std::vector<uint64_t> indices(numVoxels + 1);

            indices[0] = 0;
            for (size_t j = 1; j < indices.size(); j++) {
              indices[j] = indices[j - 1] + numTimesteps;
            }

            assert(indices.back() == numVoxels * numTimesteps);

            return vklNewData(indices.size(), VKL_ULONG, indices.data());
          }

        } else {
          return nullptr;
        }
      }

      VKLData generateUnstructuredTimesData(size_t numVoxels) const
      {
        if (hasUnstructuredTime()) {
          std::vector<float> times(numVoxels * numTimesteps);

          rkcommon::tasking::parallel_for(numVoxels, [&](size_t i) {
            for (size_t t = 0; t < timeSamples.size(); t++) {
              size_t index = numTimesteps * i + t;
              times[index] = timeSamples[t];
            }
          });

          return vklNewData(times.size(), VKL_FLOAT, times.data());
        }

        else {
          return nullptr;
        }
      }

      bool operator==(const TemporalConfig &other) const
      {
        return numTimesteps == other.numTimesteps &&
               timeSamples == other.timeSamples;
      }

      // 1 time step indicates temporally constant
      size_t numTimesteps;

      // for temporally unstructured, we currently assume the same time samples
      // for every voxel
      std::vector<float> timeSamples;
    };

    struct TestingStructuredVolume : public TestingVolume
    {
      TestingStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLDataType voxelType,
          const TemporalConfig &temporalConfig   = TemporalConfig(),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      range1f getComputedValueRange() const override;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;
      VKLDataType getVoxelType() const;
      TemporalConfig getTemporalConfig() const;

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<unsigned char> generateVoxels() = 0;

     protected:
      void generateVKLVolume() override final;

      range1f computedValueRange = range1f(rkcommon::math::empty);

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLDataType voxelType;
      TemporalConfig temporalConfig;
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;

      // data may need to be retained for shared data buffers
      std::vector<unsigned char> voxels;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolume::TestingStructuredVolume(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataType voxelType,
        const TemporalConfig &temporalConfig,
        VKLDataCreationFlags dataCreationFlags,
        size_t _byteStride)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          voxelType(voxelType),
          temporalConfig(temporalConfig),
          dataCreationFlags(dataCreationFlags),
          byteStride(_byteStride)
    {
      if (byteStride == 0) {
        byteStride = sizeOfVKLDataType(voxelType);
      }

      if (byteStride < sizeOfVKLDataType(voxelType)) {
        throw std::runtime_error("byteStride must be >= size of voxel type");
      }
    }

    inline range1f TestingStructuredVolume::getComputedValueRange() const
    {
      if (computedValueRange.empty()) {
        throw std::runtime_error(
            "computedValueRange only available after VKL volume is generated");
      }

      return computedValueRange;
    }

    inline std::string TestingStructuredVolume::getGridType() const
    {
      return gridType;
    }

    inline vec3i TestingStructuredVolume::getDimensions() const
    {
      return dimensions;
    }

    inline vec3f TestingStructuredVolume::getGridOrigin() const
    {
      return gridOrigin;
    }

    inline vec3f TestingStructuredVolume::getGridSpacing() const
    {
      return gridSpacing;
    }

    inline VKLDataType TestingStructuredVolume::getVoxelType() const
    {
      return voxelType;
    }

    inline TemporalConfig TestingStructuredVolume::getTemporalConfig() const
    {
      return temporalConfig;
    }

    inline void TestingStructuredVolume::generateVKLVolume()
    {
      voxels = generateVoxels();

      if (voxels.size() != dimensions.long_product() *
                               temporalConfig.numTimesteps * byteStride) {
        throw std::runtime_error("generated voxel data has incorrect size");
      }

      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      VKLData data =
          vklNewData(dimensions.long_product() * temporalConfig.numTimesteps,
                     voxelType,
                     voxels.data(),
                     dataCreationFlags,
                     byteStride);
      vklSetData(volume, "data", data);
      vklRelease(data);

      if (temporalConfig.hasStructuredTime()) {
        vklSetInt(volume,
                  "temporallyStructuredNumTimesteps",
                  temporalConfig.getStructuredNumTimesteps());
      } else if (temporalConfig.hasUnstructuredTime()) {
        VKLData indicesData = temporalConfig.generateUnstructuredIndicesData(
            dimensions.long_product());
        vklSetData(volume, "temporallyUnstructuredIndices", indicesData);
        vklRelease(indicesData);

        VKLData timesData = temporalConfig.generateUnstructuredTimesData(
            dimensions.long_product());
        vklSetData(volume, "temporallyUnstructuredTimes", timesData);
        vklRelease(timesData);
      }

      vklCommit(volume);

      computedValueRange = computeValueRange(
          voxelType,
          voxels.data(),
          dimensions.long_product() * temporalConfig.numTimesteps);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<unsigned char>().swap(voxels);
      }
    }

  }  // namespace testing
}  // namespace openvkl

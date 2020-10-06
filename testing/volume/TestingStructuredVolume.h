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
      TemporalConfig(uint8_t numTimesteps,
                     std::vector<float> timeSamples = std::vector<float>())
          : numTimesteps(numTimesteps), timeSamples(timeSamples)
      {
        if (!timeSamples.empty() && numTimesteps != timeSamples.size()) {
          throw std::runtime_error(
              "timeSamples size not equal to numTimesteps");
        }
      }

      uint8_t numTimesteps;
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
          const TemporalConfig &temporalConfig   = TemporalConfig(1),
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

      std::vector<uint8_t> generateTimeConfig();
      std::vector<float> generateTimeData();

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
      std::vector<uint8_t> timeConfig;
      std::vector<float> timeData;
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

    inline std::vector<uint8_t> TestingStructuredVolume::generateTimeConfig()
    {
      if (temporalConfig.timeSamples.empty()) {
        return std::vector<uint8_t>(1, temporalConfig.numTimesteps);
      }

      std::vector<uint8_t> timeConfig(this->dimensions.long_product(),
                                      temporalConfig.numTimesteps);

      return timeConfig;
    }

    inline std::vector<float> TestingStructuredVolume::generateTimeData()
    {
      if (temporalConfig.numTimesteps == 1 ||
          temporalConfig.timeSamples.empty()) {
        return std::vector<float>();
      }

      auto numValues = this->dimensions.long_product();
      std::vector<float> timeData(numValues * temporalConfig.numTimesteps);

      rkcommon::tasking::parallel_for(this->dimensions.z, [&](int z) {
        for (size_t y = 0; y < this->dimensions.y; y++) {
          for (size_t x = 0; x < this->dimensions.x; x++) {
            for (size_t t = 0; t < temporalConfig.timeSamples.size(); t++) {
              size_t index =
                  temporalConfig.numTimesteps * size_t(z) * this->dimensions.y *
                      this->dimensions.x +
                  temporalConfig.numTimesteps * y * this->dimensions.x +
                  temporalConfig.numTimesteps * x + t;
              timeData[index] = temporalConfig.timeSamples[t];
            }
          }
        }
      });

      return timeData;
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

      if (temporalConfig.numTimesteps > 1) {
        timeConfig = generateTimeConfig();
        timeData   = generateTimeData();

        VKLData attributeTimeConfig = vklNewData(timeConfig.size(),
                                                 VKL_UCHAR,
                                                 timeConfig.data(),
                                                 dataCreationFlags,
                                                 0);

        VKLData attributeTimeData = nullptr;

        if (timeData.size() > 0) {
          attributeTimeData = vklNewData(timeData.size(),
                                         VKL_FLOAT,
                                         timeData.data(),
                                         dataCreationFlags,
                                         0);
        }

        if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
          std::vector<uint8_t>().swap(timeConfig);
          std::vector<float>().swap(timeData);
        }

        VKLData timeConfigData = vklNewData(1, VKL_DATA, &attributeTimeConfig);
        VKLData timeDataData   = vklNewData(1, VKL_DATA, &attributeTimeData);

        vklRelease(attributeTimeConfig);

        if (attributeTimeData) {
          vklRelease(attributeTimeData);
        }

        vklSetData(volume, "timeConfig", timeConfigData);
        vklSetData(volume, "timeData", timeDataData);

        vklRelease(timeConfigData);
        vklRelease(timeDataData);
      }

      vklCommit(volume);

      computedValueRange = computeValueRange(
          voxelType, voxels.data(), dimensions.long_product());

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<unsigned char>().swap(voxels);
      }
    }

  }  // namespace testing
}  // namespace openvkl

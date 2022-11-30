// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
// openvkl
#include "TestingVolume.h"
// rkcommon
#include "rkcommon/math/range.h"

namespace openvkl {
  namespace testing {

    struct TestingStructuredVolume : public TestingVolume
    {
      TestingStructuredVolume() = delete;

      range1f getComputedValueRange() const override;

      std::string getGridType() const;
      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;
      VKLDataType getVoxelType() const;
      const TemporalConfig &getTemporalConfig() const;

      virtual void generateVoxels(std::vector<unsigned char> &voxels,
                                  std::vector<float> &time,
                                  std::vector<uint32_t> &tuvIndex) const = 0;

     protected:
      TestingStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const TemporalConfig &temporalConfig,
          VKLDataType voxelType,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      void generateVKLVolume(VKLDevice device) override final;

      range1f computedValueRange = range1f(rkcommon::math::empty);

      const std::string gridType;
      vec3i dimensions;
      const vec3f gridOrigin;
      const vec3f gridSpacing;
      const VKLDataType voxelType;
      const VKLDataCreationFlags dataCreationFlags;
      TemporalConfig temporalConfig;
      size_t byteStride;

     private:
      std::vector<unsigned char> voxels;
      std::vector<float> time;
      std::vector<uint32_t> tuvIndex;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolume::TestingStructuredVolume(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const TemporalConfig &temporalConfig,
        VKLDataType voxelType,
        VKLDataCreationFlags dataCreationFlags,
        size_t _byteStride)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          temporalConfig(temporalConfig),
          voxelType(voxelType),
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

    inline const TemporalConfig &TestingStructuredVolume::getTemporalConfig()
        const
    {
      return temporalConfig;
    }

    inline void TestingStructuredVolume::generateVKLVolume(VKLDevice device)
    {
      volume = vklNewVolume(device, gridType.c_str());

      generateVoxels(voxels, time, tuvIndex);

      vklSetVec3i2(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f2(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f2(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      size_t totalNumValues = 0;
      switch (temporalConfig.type) {
      case TemporalConfig::Constant: {
        totalNumValues = dimensions.long_product();

        if (voxels.size() != totalNumValues * byteStride)
          throw std::runtime_error("generated voxel data has incorrect size");
        if (!time.empty())
          throw std::runtime_error(
              "unexpected time data for temporally constant volume");
        if (!tuvIndex.empty())
          throw std::runtime_error(
              "unexpected TUV index data for temporally constant volume");

        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData2(volume, "data", data);
        vklRelease(data);
        break;
      }
      case TemporalConfig::Structured: {
        totalNumValues =
            dimensions.long_product() * temporalConfig.sampleTime.size();

        if (voxels.size() != totalNumValues * byteStride)
          throw std::runtime_error("generated voxel data has incorrect size");
        if (!time.empty())
          throw std::runtime_error(
              "unexpected time data for temporally structured volume");
        if (!tuvIndex.empty())
          throw std::runtime_error(
              "unexpected TUV index data for temporally structured volume");

        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData2(volume, "data", data);
        vklRelease(data);
        vklSetInt2(volume,
                  "temporallyStructuredNumTimesteps",
                  temporalConfig.sampleTime.size());
        break;
      }
      case TemporalConfig::Unstructured: {
        totalNumValues = tuvIndex.empty() ? 0 : *tuvIndex.rbegin();

        if (tuvIndex.size() != dimensions.long_product() + 1)
          throw std::runtime_error(
              "generated TUV index data has incorrect size");
        const size_t totalNumValues = tuvIndex.empty() ? 0 : *tuvIndex.rbegin();
        if (voxels.size() != totalNumValues * byteStride)
          throw std::runtime_error("generated voxel data has incorrect size");
        if (time.size() != totalNumValues)
          throw std::runtime_error("generated time data has incorrect size");

        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData2(volume, "data", data);
        vklRelease(data);

        VKLData indexData = vklNewData(device,
                                       tuvIndex.size(),
                                       VKL_UINT,
                                       tuvIndex.data(),
                                       dataCreationFlags);
        vklSetData2(volume, "temporallyUnstructuredIndices", indexData);
        vklRelease(indexData);

        VKLData timeData = vklNewData(
            device, time.size(), VKL_FLOAT, time.data(), dataCreationFlags);
        vklSetData2(volume, "temporallyUnstructuredTimes", timeData);
        vklRelease(timeData);

        break;
      }
      }

      vklCommit2(volume);

      computedValueRange =
          computeValueRange(voxelType, voxels.data(), totalNumValues);

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<unsigned char>().swap(voxels);
        std::vector<float>().swap(time);
        std::vector<uint32_t>().swap(tuvIndex);
      }
    }

  }  // namespace testing
}  // namespace openvkl

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
      enum Type
      {
        Constant,
        Structured,
        Unstructured,
      };

      Type type{Constant};
      std::vector<float> sampleTime;

      // If type is unstructured and this is nonzero, first sample the function
      // at sampleTime, then find a time range with nonzero density, and
      // resample outputting numRefitSamples time steps.
      size_t numRefitSamples{0};

      TemporalConfig() = default;

      TemporalConfig(Type type, size_t numSamples)
          : type(type), sampleTime(equidistantTime(numSamples))
      {
        assert(type == Constant || numSamples > 0);
      }

      explicit TemporalConfig(const std::vector<float> &sampleTime)
          : type(Unstructured), sampleTime(sampleTime)
      {
        assert(!sampleTime.empty());
      }

      bool isCompatible(const TemporalConfig &other) const {
        return (type == other.type) 
            && (sampleTime.size() == other.sampleTime.size())
            && (numRefitSamples == 0)
            && (other.numRefitSamples == 0);
      }

      bool hasTime() const {
        return type != Constant;
      }

      size_t getNumSamples() const {
        return type == Constant ? 1 : sampleTime.size();
      }

     private:
      static std::vector<float> equidistantTime(size_t numSamples)
      {
        std::vector<float> st(numSamples);
        // Initialize to {} for numSamples 0, {0} for numSamples 1,
        // and a regular grid between 0 and 1 for numSamples > 1.
        const float dt =
            1.f / static_cast<float>(std::max<size_t>(numSamples, 2) - 1);
        for (size_t i = 0; i < numSamples; ++i)
          st[i] = i * dt;
        return st;
      }
    };

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

      virtual void generateVoxels(
          std::vector<unsigned char> &voxels,
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
            size_t byteStride = 0);

      void generateVKLVolume() override final;

      range1f computedValueRange = range1f(rkcommon::math::empty);

      const std::string gridType;
      const vec3i dimensions;
      const vec3f gridOrigin;
      const vec3f gridSpacing;
      const VKLDataType voxelType;
      const VKLDataCreationFlags dataCreationFlags;
      const TemporalConfig temporalConfig;
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

    inline const TemporalConfig &TestingStructuredVolume::getTemporalConfig() const
    {
      return temporalConfig;
    }

    inline void TestingStructuredVolume::generateVKLVolume()
    {
      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      generateVoxels(voxels, time, tuvIndex);

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

        VKLData data = vklNewData(totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData(volume, "data", data);
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

        VKLData data = vklNewData(totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData(volume, "data", data);
        vklRelease(data);
        vklSetInt(volume,
                  "temporallyStructuredNumTimesteps",
                  temporalConfig.sampleTime.size());
        break;
      }
      case TemporalConfig::Unstructured: {
        totalNumValues = tuvIndex.empty() ? 0 : *tuvIndex.rbegin();

        if (tuvIndex.size() != dimensions.long_product()+1)
          throw std::runtime_error(
              "generated TUV index data has incorrect size");
        const size_t totalNumValues = tuvIndex.empty() ? 0 : *tuvIndex.rbegin();
        if (voxels.size() != totalNumValues * byteStride)
          throw std::runtime_error("generated voxel data has incorrect size");
        if (time.size() != totalNumValues)
          throw std::runtime_error("generated time data has incorrect size");

        VKLData data = vklNewData(totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
        vklSetData(volume, "data", data);
        vklRelease(data);

        VKLData indexData = vklNewData(
            tuvIndex.size(), VKL_UINT, tuvIndex.data(), dataCreationFlags);
        vklSetData(volume, "temporallyUnstructuredIndices", indexData);
        vklRelease(indexData);

        VKLData timeData =
            vklNewData(time.size(), VKL_FLOAT, time.data(), dataCreationFlags);
        vklSetData(volume, "temporallyUnstructuredTimes", timeData);
        vklRelease(timeData);

        break;
      }
      }

      vklCommit(volume);

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

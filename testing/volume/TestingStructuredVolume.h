// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <iterator>
#include <vector>
// openvkl
#include "TestingVolume.h"
// rkcommon
#include "rkcommon/math/range.h"

#ifdef OPENVKL_TESTING_GPU
#include "../apps/TestingAllocatorStl.h"
#endif

namespace openvkl {
  namespace testing {

    struct TestingStructuredVolume : public TestingVolume
    {
      TestingStructuredVolume() = delete;
      ~TestingStructuredVolume();

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

#ifdef OPENVKL_TESTING_GPU
      // convenience method when populating USM buffers
      void generateVoxels(UsmVector<unsigned char> &voxels,
                          UsmVector<float> &time,
                          UsmVector<uint32_t> &tuvIndex) const;
#endif

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

#ifdef OPENVKL_TESTING_GPU
      // the below are only used for GPU, and only when using shared (USM)
      // buffers
      TestingAllocatorStl<unsigned char> allocatorVoxels;
      UsmVector<unsigned char> usmVoxels;

      TestingAllocatorStl<float> allocatorTime;
      UsmVector<float> usmTime;

      TestingAllocatorStl<uint32_t> allocatorTuvIndex;
      UsmVector<uint32_t> usmTuvIndex;

      // for GPU using device-only shared buffers
      unsigned char *deviceOnly_data = nullptr;
#endif
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
#ifdef OPENVKL_TESTING_GPU
          ,
          // we only the allocators for shared buffers; for default buffers
          // initialize them on a dummy queue so we don't force SYCL setup if
          // not necessary
          allocatorVoxels(dataCreationFlags == VKL_DATA_SHARED_BUFFER
                              ? getSyclQueue()
                              : sycl::queue()),
          allocatorTime(dataCreationFlags == VKL_DATA_SHARED_BUFFER
                            ? getSyclQueue()
                            : sycl::queue()),
          allocatorTuvIndex(dataCreationFlags == VKL_DATA_SHARED_BUFFER
                                ? getSyclQueue()
                                : sycl::queue()),
          usmVoxels(allocatorVoxels),
          usmTime(allocatorTime),
          usmTuvIndex(allocatorTuvIndex)
#endif
    {
      if (byteStride == 0) {
        byteStride = sizeOfVKLDataType(voxelType);
      }

      if (byteStride < sizeOfVKLDataType(voxelType)) {
        throw std::runtime_error("byteStride must be >= size of voxel type");
      }
    }

    inline TestingStructuredVolume::~TestingStructuredVolume()
    {
#ifdef OPENVKL_TESTING_GPU
      if (deviceOnly_data) {
        try {
          sycl::free(deviceOnly_data, getSyclQueue());
          deviceOnly_data = nullptr;
        } catch (...) {
          // shutdownOpenVKL() may have already been called, destroying the SYCL
          // queue
        }
      }
#endif
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

#ifdef OPENVKL_TESTING_GPU
    inline void TestingStructuredVolume::generateVoxels(
        UsmVector<unsigned char> &voxels,
        UsmVector<float> &time,
        UsmVector<uint32_t> &tuvIndex) const
    {
      std::vector<unsigned char> tempVoxels;
      std::vector<float> tempTime;
      std::vector<uint32_t> tempTuvIndex;

      generateVoxels(tempVoxels, tempTime, tempTuvIndex);

      std::copy(
          tempVoxels.begin(), tempVoxels.end(), std::back_inserter(voxels));
      std::copy(tempTime.begin(), tempTime.end(), std::back_inserter(time));
      std::copy(tempTuvIndex.begin(),
                tempTuvIndex.end(),
                std::back_inserter(tuvIndex));
    }
#endif

    inline void TestingStructuredVolume::generateVKLVolume(VKLDevice device)
    {
      volume = vklNewVolume(device, gridType.c_str());

      generateVoxels(voxels, time, tuvIndex);

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
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

#ifdef OPENVKL_TESTING_GPU
        VKLData data;

        if (getUseDeviceOnlySharedBuffers()) {
          auto queue = getSyclQueue();

          if (deviceOnly_data) {
            sycl::free(deviceOnly_data, queue);
            deviceOnly_data = nullptr;
          }

          deviceOnly_data =
              sycl::malloc_device<unsigned char>(voxels.size(), queue);

          queue.memcpy(deviceOnly_data, voxels.data(), voxels.size());

          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            deviceOnly_data,
                            VKL_DATA_SHARED_BUFFER,
                            byteStride);
        }

        else if (dataCreationFlags == VKL_DATA_SHARED_BUFFER) {
          // for testing, we'll make a local USM copy from host memory, then
          // pass that as a shared buffer to VKL
          std::copy(
              voxels.begin(), voxels.end(), std::back_inserter(usmVoxels));

          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            usmVoxels.data(),
                            dataCreationFlags,
                            byteStride);

        } else {
          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            voxels.data(),
                            dataCreationFlags,
                            byteStride);
        }
#else
        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
#endif
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

#ifdef OPENVKL_TESTING_GPU
        VKLData data;

        if (dataCreationFlags == VKL_DATA_SHARED_BUFFER) {
          // for testing, we'll make a local USM copy from host memory, then
          // pass that as a shared buffer to VKL
          std::copy(
              voxels.begin(), voxels.end(), std::back_inserter(usmVoxels));

          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            usmVoxels.data(),
                            dataCreationFlags,
                            byteStride);
        } else {
          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            voxels.data(),
                            dataCreationFlags,
                            byteStride);
        }
#else
        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);
#endif
        vklSetData(volume, "data", data);
        vklRelease(data);
        vklSetInt(volume,
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

#ifdef OPENVKL_TESTING_GPU
        VKLData data;
        VKLData indexData;
        VKLData timeData;

        if (dataCreationFlags == VKL_DATA_SHARED_BUFFER) {
          // for testing, we'll make a local USM copy from host memory, then
          // pass that as a shared buffer to VKL
          std::copy(
              voxels.begin(), voxels.end(), std::back_inserter(usmVoxels));
          std::copy(tuvIndex.begin(),
                    tuvIndex.end(),
                    std::back_inserter(usmTuvIndex));
          std::copy(time.begin(), time.end(), std::back_inserter(usmTime));

          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            usmVoxels.data(),
                            dataCreationFlags,
                            byteStride);

          indexData = vklNewData(device,
                                 usmTuvIndex.size(),
                                 VKL_UINT,
                                 usmTuvIndex.data(),
                                 dataCreationFlags);

          timeData = vklNewData(device,
                                usmTime.size(),
                                VKL_FLOAT,
                                usmTime.data(),
                                dataCreationFlags);

        } else {
          data = vklNewData(device,
                            totalNumValues,
                            voxelType,
                            voxels.data(),
                            dataCreationFlags,
                            byteStride);

          indexData = vklNewData(device,
                                 tuvIndex.size(),
                                 VKL_UINT,
                                 tuvIndex.data(),
                                 dataCreationFlags);

          timeData = vklNewData(
              device, time.size(), VKL_FLOAT, time.data(), dataCreationFlags);
        }
#else
        VKLData data = vklNewData(device,
                                  totalNumValues,
                                  voxelType,
                                  voxels.data(),
                                  dataCreationFlags,
                                  byteStride);

        VKLData indexData = vklNewData(device,
                                       tuvIndex.size(),
                                       VKL_UINT,
                                       tuvIndex.data(),
                                       dataCreationFlags);

        VKLData timeData = vklNewData(
            device, time.size(), VKL_FLOAT, time.data(), dataCreationFlags);
#endif

        vklSetData(volume, "data", data);
        vklRelease(data);

        vklSetData(volume, "temporallyUnstructuredIndices", indexData);
        vklRelease(indexData);

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

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

    struct TestingStructuredVolume : public TestingVolume
    {
      TestingStructuredVolume(
          const std::string &gridType,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLDataType voxelType,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      range1f getComputedValueRange() const override;

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<unsigned char> generateVoxels() = 0;

     protected:
      void generateVKLVolume() override;

      range1f computedValueRange = range1f(rkcommon::math::empty);

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLDataType voxelType;
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;

      // voxel data may need to be retained for shared data buffers
      std::vector<unsigned char> voxels;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolume::TestingStructuredVolume(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataType voxelType,
        VKLDataCreationFlags dataCreationFlags,
        size_t _byteStride)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
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

    inline void TestingStructuredVolume::generateVKLVolume()
    {
      voxels = generateVoxels();

      if (voxels.size() != dimensions.long_product() * byteStride) {
        throw std::runtime_error("generated voxel data has incorrect size");
      }

      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      VKLData data = vklNewData(dimensions.long_product(),
                                voxelType,
                                voxels.data(),
                                dataCreationFlags,
                                byteStride);
      vklSetData(volume, "data", data);
      vklRelease(data);

      vklCommit(volume);

      computedValueRange = computeValueRange(
          voxelType, voxels.data(), dimensions.long_product());

      if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
        std::vector<unsigned char>().swap(voxels);
      }
    }

  }  // namespace testing
}  // namespace openvkl

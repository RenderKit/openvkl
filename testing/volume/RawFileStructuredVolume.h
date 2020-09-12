// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingStructuredVolume.h"
// std
#include <algorithm>
#include <fstream>

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    struct RawFileStructuredVolume : public TestingStructuredVolume
    {
      RawFileStructuredVolume(const std::string &filename,
                              const std::string &gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing,
                              VKLDataType voxelType);

      std::vector<unsigned char> generateVoxels(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override;
      std::vector<float> generateTimeData(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override {
        return std::vector<float>();
      };
      std::vector<unsigned int> generateTimeConfig(
          size_t numTimeSamples = 1,
          const std::vector<float>& timeSamples = std::vector<float>()) override {
        return std::vector<unsigned int>(1,1);
      };

     private:
      std::string filename;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline RawFileStructuredVolume::RawFileStructuredVolume(
        const std::string &filename,
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataType voxelType)
        : filename(filename),
          TestingStructuredVolume(
              gridType, dimensions, gridOrigin, gridSpacing, voxelType)
    {
    }

    inline std::vector<unsigned char> RawFileStructuredVolume::generateVoxels(
        size_t,
        const std::vector<float>&)
    {
      auto numValues = this->dimensions.long_product();
      std::vector<unsigned char> voxels(numValues *
                                        sizeOfVKLDataType(voxelType));

      std::ifstream input(filename, std::ios::binary);

      if (!input) {
        throw std::runtime_error("error opening raw volume file");
      }

      input.read(
          (char *)voxels.data(),
          this->dimensions.long_product() * sizeOfVKLDataType(voxelType));

      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }

      return voxels;
    }

  }  // namespace testing
}  // namespace openvkl

// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingStructuredVolume.h"
// std
#include <algorithm>
#include <fstream>

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    struct RawFileStructuredVolume final : public TestingStructuredVolume
    {
      RawFileStructuredVolume(const std::string &filename,
                              const std::string &gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing,
                              VKLDataType voxelType);

      void generateVoxels(std::vector<unsigned char> &voxels,
                          std::vector<float> &time,
                          std::vector<uint32_t> &tuvIndex) const override final;

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
          TestingStructuredVolume(gridType,
                                  dimensions,
                                  gridOrigin,
                                  gridSpacing,
                                  TemporalConfig(),
                                  voxelType)
    {
    }

    inline void RawFileStructuredVolume::generateVoxels(
        std::vector<unsigned char> &voxels,
        std::vector<float> &time,
        std::vector<uint32_t> &tuvIndex) const
    {
      const size_t numValues = this->dimensions.long_product();
      const size_t numBytes  = numValues * sizeOfVKLDataType(voxelType);

      voxels.resize(numBytes);
      time.clear();
      tuvIndex.clear();

      std::ifstream input(filename, std::ios::binary);

      if (!input) {
        throw std::runtime_error("error opening raw volume file");
      }

      input.read((char *)voxels.data(), numBytes);

      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }
    }

  }  // namespace testing
}  // namespace openvkl

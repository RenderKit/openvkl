// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "TestingStructuredVolume.h"
// std
#include <algorithm>
#include <fstream>

using namespace ospcommon;

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

      std::vector<unsigned char> generateVoxels() override;

     private:
      template <typename T>
      void computeVoxelRange(const void *data, size_t numValues);

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

    inline std::vector<unsigned char> RawFileStructuredVolume::generateVoxels()
    {
      auto numValues = longProduct(this->dimensions);
      std::vector<unsigned char> voxels(numValues *
                                        sizeOfVKLDataType(voxelType));

      std::ifstream input(filename, std::ios::binary);

      if (!input) {
        throw std::runtime_error("error opening raw volume file");
      }

      input.read((char *)voxels.data(),
                 longProduct(this->dimensions) * sizeOfVKLDataType(voxelType));

      if (!input.good()) {
        throw std::runtime_error("error reading raw volume file");
      }

      if (voxelType == VKL_UCHAR)
        computeVoxelRange<unsigned char>(voxels.data(), numValues);
      else if (voxelType == VKL_SHORT)
        computeVoxelRange<short>(voxels.data(), numValues);
      else if (voxelType == VKL_USHORT)
        computeVoxelRange<unsigned short>(voxels.data(), numValues);
      else if (voxelType == VKL_FLOAT)
        computeVoxelRange<float>(voxels.data(), numValues);
      else if (voxelType == VKL_DOUBLE)
        computeVoxelRange<double>(voxels.data(), numValues);

      return voxels;
    }

    template <typename T>
    inline void RawFileStructuredVolume::computeVoxelRange(const void *data,
                                                           size_t numValues)
    {
      const T *voxelsTyped = (const T *)data;

      auto minmax = std::minmax_element(voxelsTyped, voxelsTyped + numValues);

      voxelRange.lower = *minmax.first;
      voxelRange.upper = *minmax.second;
    }

  }  // namespace testing
}  // namespace openvkl

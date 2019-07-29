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

#include <vector>
#include "openvkl/openvkl.h"
#include "ospcommon/math/vec.h"

using namespace ospcommon::math;

namespace openvkl {
  namespace testing {

    inline size_t longProduct(const vec3i &dims)
    {
      return dims.x * size_t(dims.y) * dims.z;
    }

    template <typename T>
    VKLDataType getVKLDataType()
    {
      if (std::is_same<T, unsigned char>::value) {
        return VKL_UCHAR;
      } else if (std::is_same<T, short>::value) {
        return VKL_SHORT;
      } else if (std::is_same<T, unsigned short>::value) {
        return VKL_USHORT;
      } else if (std::is_same<T, float>::value) {
        return VKL_FLOAT;
      } else if (std::is_same<T, double>::value) {
        return VKL_DOUBLE;
      } else {
        return VKL_UNKNOWN;
      }
    }

    template <typename VOXEL_TYPE>
    struct TestingStructuredVolume
    {
      TestingStructuredVolume(const std::string gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing)
          : gridType(gridType),
            dimensions(dimensions),
            gridOrigin(gridOrigin),
            gridSpacing(gridSpacing)
      {
        voxelType = getVKLDataType<VOXEL_TYPE>();

        if (voxelType == VKL_UNKNOWN) {
          throw std::runtime_error(
              "unsupported VOXEL_TYPE for TestingStructuredVolume");
        }
      }

      virtual ~TestingStructuredVolume()
      {
        if (volume) {
          vklRelease(volume);
        }
      }

      inline VKLDataType getVoxelType() const
      {
        return voxelType;
      }

      inline vec3i getDimensions() const
      {
        return dimensions;
      }

      inline vec3f getGridOrigin() const
      {
        return gridOrigin;
      }

      inline vec3f getGridSpacing() const
      {
        return gridSpacing;
      }

      inline VKLVolume getVKLVolume()
      {
        if (!volume) {
          generateVKLVolume();
        }

        return volume;
      }

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<VOXEL_TYPE> generateVoxels() = 0;

     protected:
      void generateVKLVolume()
      {
        std::vector<VOXEL_TYPE> voxels = generateVoxels();

        volume = vklNewVolume(gridType.c_str());

        vklSet3i(
            volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
        vklSet3f(
            volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
        vklSet3f(
            volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

        VKLData voxelData = vklNewData(voxels.size(), voxelType, voxels.data());
        vklSetData(volume, "voxelData", voxelData);
        vklRelease(voxelData);

        vklCommit(volume);
      }

      VKLDataType voxelType;

      std::string gridType = "structured_reular";
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VKLVolume volume{nullptr};
    };

  }  // namespace testing
}  // namespace openvkl

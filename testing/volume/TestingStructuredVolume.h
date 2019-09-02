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
// openvkl
#include "openvkl/openvkl.h"
// ospcommon
#include "ospcommon/math/vec.h"

using namespace ospcommon::math;

namespace openvkl {
  namespace testing {

    inline size_t longProduct(const vec3i &dims)
    {
      return dims.x * size_t(dims.y) * dims.z;
    }

    template <typename T>
    inline VKLDataType getVKLDataType()
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

    inline size_t sizeOfVKLDataType(VKLDataType dataType)
    {
      switch (dataType) {
      case VKL_UCHAR:
        return sizeof(unsigned char);
      case VKL_SHORT:
        return sizeof(short);
      case VKL_USHORT:
        return sizeof(unsigned short);
      case VKL_FLOAT:
        return sizeof(float);
      case VKL_DOUBLE:
        return sizeof(double);
      case VKL_UNKNOWN:
        break;
      default:
        break;
      };

      throw std::runtime_error("cannot compute size of unknown VKLDataType");
    }

    struct TestingStructuredVolume
    {
      TestingStructuredVolume(const std::string &gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing,
                              VKLDataType voxelType);

      virtual ~TestingStructuredVolume();

      inline VKLDataType getVoxelType() const;

      inline vec3i getDimensions() const;

      inline vec3f getGridOrigin() const;

      inline vec3f getGridSpacing() const;

      inline VKLVolume getVKLVolume();

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<unsigned char> generateVoxels() = 0;

     protected:
      void generateVKLVolume();

      std::string gridType = "structured_reular";
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VKLDataType voxelType;

      VKLVolume volume{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingStructuredVolume::TestingStructuredVolume(
        const std::string &gridType,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataType voxelType)
        : gridType(gridType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          voxelType(voxelType)
    {
    }

    inline TestingStructuredVolume::~TestingStructuredVolume()
    {
      if (volume) {
        vklRelease(volume);
      }
    }

    inline VKLDataType TestingStructuredVolume::getVoxelType() const
    {
      return voxelType;
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

    inline VKLVolume TestingStructuredVolume::getVKLVolume()
    {
      if (!volume) {
        generateVKLVolume();
      }

      return volume;
    }

    inline void TestingStructuredVolume::generateVKLVolume()
    {
      std::vector<unsigned char> voxels = generateVoxels();

      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      VKLData voxelData =
          vklNewData(longProduct(dimensions), voxelType, voxels.data());
      vklSetData(volume, "voxelData", voxelData);
      vklRelease(voxelData);

      vklCommit(volume);
    }

  }  // namespace testing
}  // namespace openvkl

// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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
#include "TestingVolume.h"
// ospcommon
#include "ospcommon/math/range.h"

namespace openvkl {
  namespace testing {

    struct TestingStructuredVolume : public TestingVolume
    {
      TestingStructuredVolume(const std::string &gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing,
                              VKLDataType voxelType);

      range1f getComputedValueRange() const override;

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<unsigned char> generateVoxels() = 0;

     protected:
      void generateVKLVolume() override;

      range1f computedValueRange = range1f(ospcommon::math::empty);

      std::string gridType;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLDataType voxelType;
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
      std::vector<unsigned char> voxels = generateVoxels();

      volume = vklNewVolume(gridType.c_str());

      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetVec3f(
          volume, "gridOrigin", gridOrigin.x, gridOrigin.y, gridOrigin.z);
      vklSetVec3f(
          volume, "gridSpacing", gridSpacing.x, gridSpacing.y, gridSpacing.z);

      VKLData voxelData =
          vklNewData(dimensions.long_product(), voxelType, voxels.data());
      vklSetData(volume, "voxelData", voxelData);
      vklRelease(voxelData);

      vklCommit(volume);

      computedValueRange = computeValueRange(
          voxelType, voxels.data(), dimensions.long_product());
    }

  }  // namespace testing
}  // namespace openvkl

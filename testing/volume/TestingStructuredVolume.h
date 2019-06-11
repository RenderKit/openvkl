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
#include "ospray/ospcommon/vec.h"
#include "openvkl/openvkl.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    struct TestingStructuredVolume
    {
      TestingStructuredVolume(const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing)
          : dimensions(dimensions),
            gridOrigin(gridOrigin),
            gridSpacing(gridSpacing)
      {
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

      inline VKLVolume getVKLVolume() const
      {
        return volume;
      }

      // allow external access to underlying voxel data (e.g. for conversion to
      // other volume formats / types)
      virtual std::vector<float> generateVoxels() = 0;

     protected:
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;

      VKLVolume volume{nullptr};
    };

  }  // namespace testing
}  // namespace openvkl

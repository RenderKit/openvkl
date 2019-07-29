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

#include <fstream>
#include "TestingStructuredVolume.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE>
    struct RawFileStructuredVolume : public TestingStructuredVolume<VOXEL_TYPE>
    {
      RawFileStructuredVolume(const std::string &filename,
                              const std::string &gridType,
                              const vec3i &dimensions,
                              const vec3f &gridOrigin,
                              const vec3f &gridSpacing)
          : filename(filename),
            TestingStructuredVolume<VOXEL_TYPE>(
                gridType, dimensions, gridOrigin, gridSpacing)
      {
      }

      std::vector<VOXEL_TYPE> generateVoxels() override
      {
        std::vector<VOXEL_TYPE> voxels(longProduct(this->dimensions));

        std::ifstream input(filename, std::ios::binary);

        if (!input) {
          throw std::runtime_error("error opening raw volume file");
        }

        input.read((char *)voxels.data(),
                   longProduct(this->dimensions) * sizeof(VOXEL_TYPE));

        if (!input.good()) {
          throw std::runtime_error("error reading raw volume file");
        }

        return voxels;
      }

     protected:
      std::string filename;
    };

  }  // namespace testing
}  // namespace openvkl

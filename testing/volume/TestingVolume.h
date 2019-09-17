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

// openvkl
#include "openvkl/openvkl.h"
// ospcommon
#include "ospcommon/math/vec.h"

using namespace ospcommon::math;

namespace openvkl {
  namespace testing {

    ///////////////////////////////////////////////////////////////////////////
    // Helper functions ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

    ///////////////////////////////////////////////////////////////////////////
    // TestingVolume //////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    struct TestingVolume
    {
      TestingVolume() = default;
      virtual ~TestingVolume();

      VKLVolume getVKLVolume();

     protected:
      virtual void generateVKLVolume() = 0;

      VKLVolume volume{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingVolume::~TestingVolume()
    {
      if (volume) {
        vklRelease(volume);
      }
    }

    inline VKLVolume TestingVolume::getVKLVolume()
    {
      if (!volume) {
        generateVKLVolume();
      }

      return volume;
    }

  }  // namespace testing
}  // namespace openvkl

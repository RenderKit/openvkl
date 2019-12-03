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

#include "ospcommon/math/vec.h"

namespace openvkl {
  namespace testing {

    inline vec3f gradientNotImplemented(const vec3f &)
    {
      throw std::runtime_error(
          "gradient function not implemented for this procedural volume");
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE samplingNotImplemented(const vec3f &)
    {
      throw std::runtime_error(
          "sampling function not implemented for this procedural volume");
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE getWaveletValue(const vec3f &objectCoordinates)
    {
      // wavelet parameters
      constexpr double M  = 1.f;
      constexpr double G  = 1.f;
      constexpr double XM = 1.f;
      constexpr double YM = 1.f;
      constexpr double ZM = 1.f;
      constexpr double XF = 3.f;
      constexpr double YF = 3.f;
      constexpr double ZF = 3.f;

      double value = M * G *
                     (XM * ::sin(XF * objectCoordinates.x) +
                      YM * ::sin(YF * objectCoordinates.y) +
                      ZM * ::cos(ZF * objectCoordinates.z));

      if (std::is_unsigned<VOXEL_TYPE>::value) {
        value = fabs(value);
      }

      value = clamp(value,
                    double(std::numeric_limits<VOXEL_TYPE>::lowest()),
                    double(std::numeric_limits<VOXEL_TYPE>::max()));

      return VOXEL_TYPE(value);
    }

    inline vec3f getWaveletGradient(const vec3f &objectCoordinates)
    {
      // wavelet parameters
      constexpr double M  = 1.f;
      constexpr double G  = 1.f;
      constexpr double XM = 1.f;
      constexpr double YM = 1.f;
      constexpr double ZM = 1.f;
      constexpr double XF = 3.f;
      constexpr double YF = 3.f;
      constexpr double ZF = 3.f;

      return M * G *
             vec3f(XM * ::cos(XF * objectCoordinates.x) * XF,
                   YM * ::cos(YF * objectCoordinates.y) * YF,
                   -ZM * ::sin(ZF * objectCoordinates.z) * ZF);
    }

    inline float getShellValue(const vec3f &objectCoordinates, const vec3i &dimensions)
    {
        if(box3i(7 * dimensions / 16, 9 * dimensions / 16).contains(objectCoordinates))
            return 1.0f;
        else if (box3i(dimensions / 4, 3 * dimensions / 4).contains(objectCoordinates))
            return 0.0f;
        else
            return -.5f;
    }

    inline float getZValue(const vec3f &objectCoordinates)
    {
      return objectCoordinates.z;
    }

    inline vec3f getZGradient(const vec3f &objectCoordinates)
    {
      return vec3f(0.f, 0.f, 1.f);
    }

    inline float getXYZValue(const vec3f &objectCoordinates)
    {
      return objectCoordinates.x * objectCoordinates.y * objectCoordinates.z;
    }

    inline vec3f getXYZGradient(const vec3f &objectCoordinates)
    {
      return vec3f(objectCoordinates.y * objectCoordinates.z,
                   objectCoordinates.x * objectCoordinates.z,
                   objectCoordinates.x * objectCoordinates.y);
    }

    inline float getConstValue(const vec3f &objectCoordinates)
    {
      return 0.5f;
    }

    inline vec3f getConstGradient(const vec3f &objectCoordinates)
    {
      return vec3f(0.f);
    }

    inline float getRadiusValue(const vec3f &objectCoordinates)
    {
      return sqrtf(objectCoordinates.x * objectCoordinates.x +
                   objectCoordinates.y * objectCoordinates.y +
                   objectCoordinates.z * objectCoordinates.z);
    }

  }  // namespace testing
}  // namespace openvkl

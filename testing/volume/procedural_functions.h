// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace testing {

    inline vec3f gradientNotImplemented(const vec3f &, float)
    {
      throw std::runtime_error(
          "gradient function not implemented for this procedural volume");
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE samplingNotImplemented(const vec3f &, float)
    {
      throw std::runtime_error(
          "sampling function not implemented for this procedural volume");
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE getWaveletValue(const vec3f &objectCoordinates,
                                      float time)
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

      // for exact temporal functional tests, this needs to vary linearly with
      // time
      double value = M * G *
                         (XM * ::sin(XF * objectCoordinates.x) +
                          YM * ::sin(YF * objectCoordinates.y) +
                          ZM * ::cos(ZF * objectCoordinates.z)) +
                     objectCoordinates.x * time;

      if (std::is_unsigned<VOXEL_TYPE>::value) {
        value = fabs(value);
      }

      value = clamp(value,
                    double(std::numeric_limits<VOXEL_TYPE>::lowest()),
                    double(std::numeric_limits<VOXEL_TYPE>::max()));

      return VOXEL_TYPE(value);
    }

    inline vec3f getWaveletGradient(const vec3f &objectCoordinates, float time)
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
                       -ZM * ::sin(ZF * objectCoordinates.z) * ZF) +
             vec3f(time, 0.f, 0.f);
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE getXYZValue(const vec3f &objectCoordinates, float time)
    {
      double value = (1.f - time) * objectCoordinates.x * objectCoordinates.y *
                     objectCoordinates.z;

      if (std::is_unsigned<VOXEL_TYPE>::value) {
        value = fabs(value);
      }

      value = clamp(value,
                    double(std::numeric_limits<VOXEL_TYPE>::lowest()),
                    double(std::numeric_limits<VOXEL_TYPE>::max()));

      return VOXEL_TYPE(value);
    }

    inline vec3f getXYZGradient(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * vec3f(objectCoordinates.y * objectCoordinates.z,
                                  objectCoordinates.x * objectCoordinates.z,
                                  objectCoordinates.x * objectCoordinates.y);
    }

    inline float getShellValue(const vec3f &objectCoordinates,
                               const vec3i &dimensions)
    {
      if (box3i(7 * dimensions / 16, 9 * dimensions / 16)
              .contains(objectCoordinates))
        return 1.0f;
      else if (box3i(dimensions / 4, 3 * dimensions / 4)
                   .contains(objectCoordinates))
        return 0.0f;
      else
        return -.5f;
    }

    inline float getXValue(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * objectCoordinates.x;
    }

    inline vec3f getXGradient(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * vec3f(1.f, 0.f, 0.f);
    }

    inline float getYValue(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * objectCoordinates.y;
    }

    inline vec3f getYGradient(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * vec3f(0.f, 1.f, 0.f);
    }

    inline float getZValue(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * objectCoordinates.z;
    }

    inline vec3f getZGradient(const vec3f &objectCoordinates, float time)
    {
      return (1.f - time) * vec3f(0.f, 0.f, 1.f);
    }

    inline float getConstValue(const vec3f &objectCoordinates, float time)
    {
      return 0.5f;
    }

    inline vec3f getConstGradient(const vec3f &objectCoordinates, float time)
    {
      return vec3f(0.f);
    }

    template <typename VOXEL_TYPE>
    inline VOXEL_TYPE getRotatingSphereValue(const vec3f &objectCoordinates,
                                             float time)
    {
      constexpr float cr     = 0.6f;
      constexpr float radius = 0.1f;
      const float phase      = time * 4.f * M_PI;
      const vec3f center{cr * sin(phase), cr * cos(phase), 0.2f + 0.6f * time};
      const float radSq = radius * radius;
      const vec3f d     = objectCoordinates - center;
      const float rr    = dot(d, d);
      return (rr < radSq) ? VOXEL_TYPE{1} : VOXEL_TYPE{0};
    }

    inline vec3f getRotatingSphereGradient(const vec3f &objectCoordinates,
                                           float time)
    {
      return vec3f(0.f);
    }

  }  // namespace testing
}  // namespace openvkl

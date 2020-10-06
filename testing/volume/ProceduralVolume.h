// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace testing {

    struct ProceduralVolume
    {
      ProceduralVolume(bool supportsTime) : supportsTime(supportsTime) {}

      // these public non-virtual methods handle all parameter defaults

      float computeProceduralValue(const vec3f &objectCoordinates,
                                   float time = 0.f) const;

      vec3f computeProceduralGradient(const vec3f &objectCoordinates,
                                      float time = 0.f) const;

     protected:
      virtual float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                               float time) const = 0;

      virtual vec3f computeProceduralGradientImpl(
          const vec3f &objectCoordinates) const = 0;

     private:
      bool supportsTime;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline float ProceduralVolume::computeProceduralValue(
        const vec3f &objectCoordinates, float time) const
    {
      if (!supportsTime && time != 0.f) {
        throw std::runtime_error(
            "computeProceduralValue() called with non-zero time on "
            "incompatible volume");
      }

      return computeProceduralValueImpl(objectCoordinates, time);
    }

    inline vec3f ProceduralVolume::computeProceduralGradient(
        const vec3f &objectCoordinates, float time) const
    {
      if (time != 0.f) {
        throw std::runtime_error(
            "time not yet considered for procedural gradients");
      }

      return computeProceduralGradientImpl(objectCoordinates);
    }
  }  // namespace testing
}  // namespace openvkl

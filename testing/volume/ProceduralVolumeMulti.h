// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace testing {

    inline std::vector<unsigned int> getAttributeIndices(
        unsigned int numAttributes)
    {
      std::vector<unsigned int> attributeIndices(numAttributes);
      std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

      return attributeIndices;
    }

    struct ProceduralVolumeMulti
    {
      ProceduralVolumeMulti(bool supportsTime) : supportsTime(supportsTime) {}

      virtual unsigned int getNumAttributes() const = 0;

      // these public non-virtual methods handle all parameter defaults

      float computeProceduralValue(const vec3f &objectCoordinates,
                                   unsigned int attributeIndex,
                                   float time = 0.f) const;

      vec3f computeProceduralGradient(const vec3f &objectCoordinates,
                                      unsigned int attributeIndex,
                                      float time = 0.f) const;

     protected:
      virtual float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                               unsigned int attributeIndex,
                                               float time) const = 0;

      virtual vec3f computeProceduralGradientImpl(
          const vec3f &objectCoordinates,
          unsigned int attributeIndex,
          float time) const = 0;

     private:
      bool supportsTime;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline float ProceduralVolumeMulti::computeProceduralValue(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      if (!supportsTime && time != 0.f) {
        throw std::runtime_error(
            "computeProceduralValue() called with non-zero time on "
            "incompatible volume");
      }

      return computeProceduralValueImpl(
          objectCoordinates, attributeIndex, time);
    }

    inline vec3f ProceduralVolumeMulti::computeProceduralGradient(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      if (!supportsTime && time != 0.f) {
        throw std::runtime_error(
            "computeProceduralGradient() called with non-zero time on "
            "incompatible volume");
      }

      return computeProceduralGradientImpl(
          objectCoordinates, attributeIndex, time);
    }
  }  // namespace testing
}  // namespace openvkl

// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace testing {

    struct ProceduralVolume
    {
      virtual float computeProceduralValue(
          const vec3f &objectCoordinates) const = 0;
      virtual vec3f computeProceduralGradient(
          const vec3f &objectCoordinates) const = 0;
    };

  }  // namespace testing
}  // namespace openvkl

// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../cpu/common/ValueRangesShared.h"
#include "../../cpu/math/box_utility.ih"

namespace ispc {

  // from Interval.ih; we use this instead of VKLInterval for box1f types
  struct Interval
  {
    box1f tRange;
    box1f valueRange;
    float nominalDeltaT;
  };

  inline void resetInterval(Interval &interval)
  {
    interval.tRange.lower     = 1.f;
    interval.tRange.upper     = -1.f;
    interval.valueRange.lower = 0.f;
    interval.valueRange.upper = 0.f;
    interval.nominalDeltaT    = 0.f;
  }

  // from ValueRanges.ih
  inline bool valueRangesOverlap(const ValueRanges &valueRanges, const box1f &r)
  {
    if (valueRanges.numRanges == 0) {
      return true;
    }

    if (overlaps1f(valueRanges.rangesMinMax, r)) {
      for (int i = 0; i < valueRanges.numRanges; i++) {
        if (overlaps1f(valueRanges.ranges[i], r)) {
          return true;
        }
      }
    }

    return false;
  }
}  // namespace ispc
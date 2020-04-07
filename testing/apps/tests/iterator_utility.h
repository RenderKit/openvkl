// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl_testing.h"

inline vkl_range1f computeIntervalValueRange(VKLVolume volume,
                                             const vkl_vec3f &origin,
                                             const vkl_vec3f &direction,
                                             const vkl_range1f &tRange)
{
  vkl_range1f sampledValueRange{inf, neg_inf};

  constexpr int numValueRangeSamples = 100;

  for (int i = 0; i < numValueRangeSamples; i++) {
    float t = tRange.lower + float(i) / float(numValueRangeSamples - 1) *
                                 (tRange.upper - tRange.lower);

    vkl_vec3f c{origin.x + t * direction.x,
                origin.y + t * direction.y,
                origin.z + t * direction.z};

    float sample = vklComputeSample(volume, &c);

    sampledValueRange.lower = std::min(sampledValueRange.lower, sample);
    sampledValueRange.upper = std::max(sampledValueRange.upper, sample);
  }

  return sampledValueRange;
}

inline bool rangesIntersect(const vkl_range1f &range1,
                            const vkl_range1f &range2)
{
  return range1.upper >= range2.lower && range1.lower <= range2.upper;
}

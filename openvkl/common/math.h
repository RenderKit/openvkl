// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>

namespace openvkl {

  inline std::pair<float, float> intersectBox(const vec3f &origin,
                                              const vec3f &direction,
                                              const box3f &box,
                                              const range1f &rangeLimit)
  {
    const vec3f mins = (box.lower - origin) * rcp(direction);
    const vec3f maxs = (box.upper - origin) * rcp(direction);

    const vec3f min1 = min(mins, maxs);
    const vec3f max1 = max(mins, maxs);

    return {reduce_max(vec4f{min1.x, min1.y, min1.z, rangeLimit.lower}),
            reduce_min(vec4f{max1.x, max1.y, max1.z, rangeLimit.upper})};
  }

  inline vec3f nextafter(const vec3i &a, const vec3i &b)
  {
    return vec3f(
        nextafterf(a.x, b.x), nextafterf(a.y, b.y), nextafterf(a.z, b.z));
  }

}  // namespace openvkl

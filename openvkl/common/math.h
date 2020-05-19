// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"

namespace openvkl {

  using namespace rkcommon::math;

  inline std::pair<float, float> intersectBox(const vec3f &origin,
                                              const vec3f &direction,
                                              const box3f &box,
                                              const range1f &rangeLimit)
  {
    const vec3f mins = (box.lower - origin) * rcp(direction);
    const vec3f maxs = (box.upper - origin) * rcp(direction);

    return {reduce_max(vec4f{min(mins, maxs), rangeLimit.lower}),
            reduce_min(vec4f{max(mins, maxs), rangeLimit.upper})};
  }

  inline vec3f nextafter(const vec3i &a, const vec3i &b)
  {
    return vec3f(
        nextafterf(a.x, b.x), nextafterf(a.y, b.y), nextafterf(a.z, b.z));
  }

}  // namespace openvkl

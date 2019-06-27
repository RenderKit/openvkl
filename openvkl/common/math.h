// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include <cmath>
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"

namespace openvkl {

  using namespace ospcommon::math;

  inline size_t longProduct(const vec3i &dims)
  {
    return dims.x * size_t(dims.y) * dims.z;
  }

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

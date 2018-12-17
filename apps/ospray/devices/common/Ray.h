// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include <ospray/ospcommon/box.h>
#include <ospray/ospcommon/vec.h>

namespace ospray {
  namespace scalar_volley_device {

    using namespace ospcommon;

    struct Ray
    {
      vec3f org;
      vec3f dir;
      float t0{0.f};
      float t{inf};
    };

    inline std::pair<float, float> intersectBox(const Ray &ray,
                                                const box3f &box)
    {
      const vec3f mins = (box.lower - ray.org) * rcp(ray.dir);
      const vec3f maxs = (box.upper - ray.org) * rcp(ray.dir);

      return {reduce_max(vec4f{min(mins, maxs), ray.t0}),
              reduce_min(vec4f{max(mins, maxs), ray.t})};
    }

  }  // namespace scalar_volley_device
}  // namespace ospray

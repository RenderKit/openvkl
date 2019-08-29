// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "IntersectBounds.h"

namespace openvkl {
  namespace examples {

    vec3f IntersectBounds::renderPixel(VKLVolume volume,
                                       const box3f &volumeBounds,
                                       VKLSamplesMask,
                                       Ray &ray)
    {
      const auto results = intersectRayBox(ray.org, ray.dir, volumeBounds);
      return results.empty() ? vec3f(0.f) : vec3f(1.f);
    }

  }  // namespace examples
}  // namespace openvkl
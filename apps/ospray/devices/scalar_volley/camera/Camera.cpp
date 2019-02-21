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

#include "Camera.h"

namespace ospray {
  namespace scalar_volley_device {

    void Camera::commit()
    {
      pos = getParam<vec3f>("pos", pos);
      dir = normalize(getParam<vec3f>("dir", dir));
      up  = normalize(getParam<vec3f>("up", up));

      float fovy = getParam<float>("fovy", 60.0);
      float aspect = getParam<float>("aspect", 1.f);

      dir_du = normalize(cross(dir, up));
      dir_dv = cross(dir_du, dir);

      float imgPlane_size_y = 2.f * tanf(fovy / 2.f * M_PI / 180.);
      float imgPlane_size_x = imgPlane_size_y * aspect;

      dir_du *= imgPlane_size_x;
      dir_dv *= imgPlane_size_y;

      dir_00 = dir - .5f * dir_du - .5f * dir_dv;
    }

    void Camera::getRay(const CameraSample &sample, Ray &ray) const
    {
      vec3f org = pos;
      vec3f dir = dir_00 + sample.screen.x * dir_du + sample.screen.y * dir_dv;

      ray.org = org;
      ray.dir = normalize(dir);
      ray.t0  = 0.f;
      ray.t   = ospcommon::inf;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray

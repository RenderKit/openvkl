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

#include "../common/ManagedObject.h"
#include "../common/Ray.h"

namespace ospray {
  namespace scalar_volley_device {

    struct CameraSample
    {
      // normalized screen sample, from [0..1]
      vec2f screen;
    };

    struct Camera : public ManagedObject
    {
      Camera() = default;
      ~Camera() = default;

      // should be run after changing any parameters
      void commit();

      void getRay(const CameraSample &cameraSample, Ray &ray) const;

     protected:
      // user-specified values; commit() should be called after changing
      vec3f pos{0.f, 0.f, 4.f};
      vec3f dir{0.f, 0.f, -1.f};
      vec3f up{0.f, 1.f, 0.f};
      float fovy{60.f};
      float aspect{1.f};

      // computed values after a commit()
      vec3f dir_00;
      vec3f dir_du;
      vec3f dir_dv;
    };

  }  // namespace scalar_volley_device
}  // namespace ospray

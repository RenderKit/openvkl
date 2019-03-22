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

#include "Volume.h"
#include "common/math.h"

namespace volley {
  namespace ispc_driver {

    struct StructuredVolume : public Volume
    {
      virtual void commit() override
      {
        Volume::commit();

        samplingMethod = VLYSamplingMethod(
            getParam<int>("samplingMethod", VLY_SAMPLE_LINEAR));
        dimensions  = getParam<vec3i>("dimensions", vec3i(128));
        gridOrigin  = getParam<vec3f>("gridOrigin", vec3f(0.f));
        gridSpacing = getParam<vec3f>("gridSpacing", vec3f(1.f));
      }

      box3f getBoundingBox() const override
      {
        return box3f(gridOrigin, gridOrigin + (dimensions-1) * gridSpacing);
      }

     protected:
      // parameters set in commit()
      VLYSamplingMethod samplingMethod;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
    };

  }  // namespace ispc_driver
}  // namespace volley

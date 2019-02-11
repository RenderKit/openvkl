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

#include "RayIterator.h"

namespace volley {
  namespace scalar_driver {

    struct DumbRayIterator : public RayIterator
    {
      DumbRayIterator(const volley::scalar_driver::Volume *volume,
                      const vec3f &origin,
                      const vec3f &direction,
                      const range1f &tRange,
                      const SamplesMask *samplesMask)
          : RayIterator(volume, origin, direction, tRange, samplesMask)
      {
      }

      bool iterateInterval() override
      {
        static float nominalDeltaT = 0.1f;

        if (currentTRange.empty()) {
          currentTRange.lower = tRange.lower;
        } else {
          currentTRange.lower += nominalDeltaT;
        }

        currentTRange.upper = currentTRange.lower + nominalDeltaT;

        return (currentTRange.lower < tRange.upper);
      }
    };

  }  // namespace scalar_driver
}  // namespace volley

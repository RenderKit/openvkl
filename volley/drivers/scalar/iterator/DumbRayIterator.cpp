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

#include "DumbRayIterator.h"
#include "../volume/Volume.h"
#include "common/math.h"

namespace volley {
  namespace scalar_driver {

    DumbRayIterator::DumbRayIterator(const Volume *volume,
                                     const vec3f &origin,
                                     const vec3f &direction,
                                     const range1f &tRange,
                                     const SamplesMask *samplesMask)
        : RayIterator(volume, origin, direction, tRange, samplesMask)
    {
      box3f boundingBox = volume->getBoundingBox();

      auto hits = intersectBox(origin, direction, boundingBox, tRange);

      if (hits.first < hits.second) {
        boundingBoxTRange.lower = hits.first;
        boundingBoxTRange.upper = hits.second;
      }
    }

    bool DumbRayIterator::iterateInterval()
    {
      if (boundingBoxTRange.empty()) {
        return false;
      }

      static float nominalDeltaT = 0.25f;

      if (currentRayInterval.tRange.empty()) {
        currentRayInterval.tRange.lower = boundingBoxTRange.lower;
      } else {
        currentRayInterval.tRange.lower += nominalDeltaT;
      }

      currentRayInterval.tRange.upper =
          std::min(currentRayInterval.tRange.lower + nominalDeltaT,
                   boundingBoxTRange.upper);

      currentRayInterval.nominalDeltaT = 0.1f;

      return (currentRayInterval.tRange.lower < boundingBoxTRange.upper);
    }

  }  // namespace scalar_driver
}  // namespace volley

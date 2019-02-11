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

#include <ospcommon/range.h>
#include "../samples_mask/SamplesMask.h"
#include "common/ManagedObject.h"

using namespace ospcommon;

namespace volley {
  namespace scalar_driver {

    struct Volume;

    struct RayIterator : public ManagedObject
    {
      RayIterator(const volley::scalar_driver::Volume *volume,
                  const vec3f &origin,
                  const vec3f &direction,
                  const range1f &tRange,
                  const SamplesMask *samplesMask)
          : volume(volume),
            origin(origin),
            direction(direction),
            tRange(tRange),
            samplesMask(samplesMask)
      {
      }

      virtual ~RayIterator() override = default;

      range1f getCurrentTRange()
      {
        return currentTRange;
      }

      virtual bool iterateInterval() = 0;

     protected:
      // initial state
      const Volume *volume;
      const vec3f origin;
      const vec3f direction;
      const range1f tRange;
      const SamplesMask *samplesMask;

      // current state
      range1f currentTRange = ospcommon::empty;
      SamplesMask intervalSamplesMask;
    };

  }  // namespace scalar_driver
}  // namespace volley

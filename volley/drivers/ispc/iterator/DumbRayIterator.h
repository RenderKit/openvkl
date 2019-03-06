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
  namespace ispc_driver {

    struct Volume;

    struct DumbRayIterator : public RayIterator
    {
      DumbRayIterator(const Volume *volume,
                      const vec3f &origin,
                      const vec3f &direction,
                      const range1f &tRange,
                      const SamplesMask *samplesMask);

      bool iterateInterval() override;

     protected:
      range1f boundingBoxTRange = ospcommon::empty;
    };

  }  // namespace ispc_driver
}  // namespace volley

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

// ospcommon
#include <ospray/ospcommon/containers/AlignedVector.h>
// scalar_volley_device
#include "Volume.h"

#include <volley/volley.h>

namespace ospray {
  namespace scalar_volley_device {

    struct VolleySimpleProceduralVolume : public Volume
    {
      VolleySimpleProceduralVolume()  = default;
      ~VolleySimpleProceduralVolume() = default;

      void commit() override;

      // returns true if the given ray intersects the volume and sets the ray t
      // values as appropriate
      bool intersect(Ray &ray) const override;

      // computes a sample at the world-space coordinate within the volume
      float computeSample(const vec3f &worldCoordinates) const override;

      // advance the given ray by a step appropriate to the volume
      void advance(Ray &ray) const override;

      int setRegion(const void *source,
                    const vec3i &index,
                    const vec3i &count) override;

     private:
      VLYVolume vlyVolume = nullptr;
    };

  }  // namespace scalar_volley_device
}  // namespace ospray

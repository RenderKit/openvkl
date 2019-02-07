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
  namespace scalar_driver {

    struct StructuredVolume : public Volume
    {
      virtual void commit() override;
      float computeSample(const vec3f &objectCoordinates) const override;
      vec3f computeGradient(const vec3f &objectCoordinates) const override;

      box3f getBoundingBox() const override
      {
        return box3f(gridOrigin, gridOrigin + dimensions * gridSpacing);
      }

     protected:
      virtual float getVoxel(const vec3i &index) const = 0;

      vec3f transformLocalToObject(const vec3f &localCoordinates) const
      {
        return gridOrigin + localCoordinates * gridSpacing;
      }

      vec3f transformObjectToLocal(const vec3f &objectCoordinates) const
      {
        return rcp(gridSpacing) * (objectCoordinates - gridOrigin);
      }

      // parameters set in commit()
      VLYSamplingMethod samplingMethod;
      vec3f gridOrigin;  // TODO: replace with more general affine transform
      vec3f gridSpacing;

      // parameters must be set in derived class commit()s
      vec3i dimensions;
      vec3f localCoordinatesUpperBound;
    };

  }  // namespace scalar_driver
}  // namespace volley
